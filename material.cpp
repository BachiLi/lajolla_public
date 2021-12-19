#include "material.h"
#include "intersection.h"
#include "microfacet.h"

inline Vector3 sample_cos_hemisphere(const Vector2 &rnd_param) {
    Real phi = c_TWOPI * rnd_param[0];
    Real tmp = sqrt(std::clamp(1 - rnd_param[1], Real(0), Real(1)));
    return Vector3{
        cos(phi) * tmp, sin(phi) * tmp,
        sqrt(std::clamp(rnd_param[1], Real(0), Real(1)))
    };
}

////////////////////////////////////////////////////////////////////////
struct eval_op {
    Spectrum operator()(const Lambertian &bsdf) const;
    Spectrum operator()(const RoughPlastic &bsdf) const;
    Spectrum operator()(const RoughDielectric &bsdf) const;

    const Vector3 &dir_in;
    const Vector3 &dir_out;
    const PathVertex &vertex;
    const TexturePool &texture_pool;
    const TransportDirection &dir;
};
Spectrum eval_op::operator()(const Lambertian &bsdf) const {
    if (dot(dir_in, vertex.shading_frame.n) < 0) {
        // Incoming direction is below the surface.
        return make_zero_spectrum();
    }
    return fmax(dot(dir_out, vertex.shading_frame.n), Real(0)) * 
           eval(bsdf.reflectance, vertex.uv, vertex.uv_screen_size, texture_pool) / c_PI;
}
Spectrum eval_op::operator()(const RoughPlastic &bsdf) const {
    Real n_dot_in = dot(dir_in, vertex.shading_frame.n);
    Real n_dot_out = dot(dir_out, vertex.shading_frame.n);
    if (n_dot_in <= 0 || n_dot_out <= 0) {
        // No light on the other side.
        return make_zero_spectrum();
    }
    Spectrum Kd = eval(
        bsdf.diffuse_reflectance, vertex.uv, vertex.uv_screen_size, texture_pool);
    Spectrum Ks = eval(
        bsdf.specular_reflectance, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real roughness = eval(
        bsdf.roughness, vertex.uv, vertex.uv_screen_size, texture_pool);
    // Clamp roughness to avoid numerical issues.
    roughness = std::clamp(roughness, Real(0.01), Real(1));
    // We first account for the dielectric layer.

    // The half-vector is a crucial component of the microfacet models.
    // Since microfacet assumes that the surface is made of many small mirrors/glasses,
    // The "average" between input and output direction determines the orientation
    // of the mirror our ray hits (since applying reflection of dir_in over half_vector
    // gives us dir_out). Microfacet models build all sorts of quantities based on the
    // half vector. It's also called the "micro normal".
    Vector3 half_vector = normalize(dir_in + dir_out);

    // Fresnel equation determines how much light goes through, 
    // and how much light is reflected for each wavelength.
    // Fresnel equation is determined by the angle between the (micro) normal and 
    // both incoming and outgoing directions (dir_out & dir_in).
    // However, since they are related through the Snell-Descartes law,
    // we only need one of them.
    Real F_o = fresnel_dielectric(dot(half_vector, dir_out), bsdf.eta); // F_o is the reflection percentage.
    Real n_dot_h = dot(half_vector, vertex.shading_frame.n);
    Real D = GTR2(n_dot_h, roughness); // "Generalized Trowbridge Reitz", GTR2 is equivalent to GGX.
    Real G = smith_masking(to_local(vertex.shading_frame, dir_in), roughness) *
             smith_masking(to_local(vertex.shading_frame, dir_out), roughness);

    Spectrum spec_contrib = Ks * (G * F_o * D) / (4 * n_dot_in * n_dot_out);

    // Next we account for the diffuse layer.
    // In order to reflect from the diffuse layer,
    // the photon needs to bounce through the dielectric layers twice.
    // The transmittance is computed by 1 - fresnel.
    Real F_i = fresnel_dielectric(dot(half_vector, dir_in), bsdf.eta);
    // Multiplying with Fresnels leads to an overly dark appearance at the 
    // object boundaries. Disney BRDF proposes a fix to this -- we will implement this in problem set 1.
    Spectrum diffuse_contrib = Kd * (Real(1) - F_o) * (Real(1) - F_i) / c_PI;

    return (spec_contrib + diffuse_contrib) * n_dot_out;
}
Spectrum eval_op::operator()(const RoughDielectric &bsdf) const {
    Spectrum Ks = eval(
        bsdf.specular_reflectance, vertex.uv, vertex.uv_screen_size, texture_pool);
    Spectrum Kt = eval(
        bsdf.specular_reflectance, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real roughness = eval(
        bsdf.roughness, vertex.uv, vertex.uv_screen_size, texture_pool);

    Vector3 n = vertex.shading_frame.n;
    Real n_dot_in = dot(dir_in, n);
    Real n_dot_out = dot(dir_out, n);
    // If we are going into the surface, then we use normal eta
    // (internal/external), otherwise we use external/internal.
    Real eta = n_dot_in > 0 ? bsdf.eta : 1 / bsdf.eta;
    bool reflect = n_dot_in * n_dot_out > 0;
    Vector3 half_vector;
    if (reflect) {
        half_vector = normalize(dir_in + dir_out);
    } else {
        // "Generalized half-vector" from Walter et al.
        // See "Microfacet Models for Refraction through Rough Surfaces"
        half_vector = normalize(dir_in + dir_out * eta);
    }

    // Flip half-vector if it's below surface
    if (dot(half_vector, vertex.shading_frame.n) < 0) {
        half_vector = -half_vector;
    }

    // Clamp roughness to avoid numerical issues.
    roughness = std::clamp(roughness, Real(0.01), Real(1));

    // Compute F / D / G
    Real h_dot_in = dot(half_vector, dir_in);
    // Note that we use the incoming direction
    // for evaluating the Fresnel reflection amount.
    // We can also use outgoing direction -- then we would need to
    // use 1/bsdf.eta and we will get the same result.
    // However, using the incoming direction allows
    // us to use F to decide whether to reflect or refract during sampling.
    Real F = fresnel_dielectric(h_dot_in, eta);
    Real D = GTR2(dot(half_vector, n), roughness);
    Real G = smith_masking(to_local(vertex.shading_frame, dir_in), roughness) *
             smith_masking(to_local(vertex.shading_frame, dir_out), roughness);
    if (reflect) {
        return Ks * (F * D * G) / (4 * fabs(n_dot_in));
    } else {
        // Snell-Descartes law predicts that the light will contract/expand 
        // due to the different index of refraction. So the normal BSDF needs
        // to scale with 1/eta^2. However, the "adjoint" of the BSDF does not have
        // the eta term. This is due to the non-reciprocal nature of the index of refraction:
        // f(wi -> wo) / eta_o^2 = f(wo -> wi) / eta_i^2
        // thus f(wi -> wo) = f(wo -> wi) (eta_o / eta_i)^2
        // The adjoint of a BSDF is defined as swapping the parameter, and
        // this cancels out the eta term.
        // See Chapter 5 of Eric Veach's thesis "Robust Monte Carlo Methods for Light Transport Simulation"
        // for more details.
        Real eta_factor = dir == TransportDirection::TO_LIGHT ? (1 / (eta * eta)) : 1;
        Real h_dot_out = dot(half_vector, dir_out);
        Real sqrt_denom = h_dot_in + eta * h_dot_out;
        // Very complicated BSDF. See Walter et al.'s paper for more details.
        // "Microfacet Models for Refraction through Rough Surfaces"
        return Kt * (eta_factor * (1 - F) * D * G * eta * eta * fabs(h_dot_out * h_dot_in)) / 
            (fabs(n_dot_in) * sqrt_denom * sqrt_denom);
    }
}
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
struct pdf_sample_bsdf_op {
    Real operator()(const Lambertian &bsdf) const;
    Real operator()(const RoughPlastic &bsdf) const;
    Real operator()(const RoughDielectric &bsdf) const;

    const Vector3 &dir_in;
    const Vector3 &dir_out;
    const PathVertex &vertex;
    const TexturePool &texture_pool;
    const TransportDirection &dir;
};
Real pdf_sample_bsdf_op::operator()(const Lambertian &bsdf) const {
    // For Lambertian, we importance sample the cosine hemisphere domain.
    Vector3 n = vertex.shading_frame.n;
    Real n_dot_in = dot(dir_in, n);
    Real n_dot_out = dot(dir_out, n);
    if (n_dot_in <= 0 || n_dot_out <= 0) {
        // No light on the other side.
        return 0;
    }

    return fmax(n_dot_out, Real(0)) / c_PI;
}
Real pdf_sample_bsdf_op::operator()(const RoughPlastic &bsdf) const {
    Vector3 n = vertex.shading_frame.n;
    Real n_dot_in = dot(dir_in, n);
    Real n_dot_out = dot(dir_out, n);
    if (n_dot_in <= 0 || n_dot_out <= 0) {
        // No light on the other side.
        return 0;
    }
    Spectrum S = eval(
        bsdf.specular_reflectance, vertex.uv, vertex.uv_screen_size, texture_pool);
    Spectrum R = eval(
        bsdf.diffuse_reflectance, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real lS = luminance(S), lR = luminance(R);
    if (lS + lR <= 0) {
        return 0;
    }
    Real roughness = eval(bsdf.roughness, vertex.uv, vertex.uv_screen_size, texture_pool);
    // Clamp roughness to avoid numerical issues.
    roughness = std::clamp(roughness, Real(0.01), Real(1));
    // We use the reflectance to determine whether to choose specular sampling lobe or diffuse.
    Real spec_prob = lS / (lS + lR);
    Real diff_prob = 1 - spec_prob;
    // For the specular lobe, we use the ellipsoidal sampling from Heitz 2018
    // "Sampling the GGX Distribution of Visible Normals"
    // https://jcgt.org/published/0007/04/01/
    // this importance samples smith_masking(cos_theta_in) * GTR2(cos_theta_h, roughness) * cos_theta_out
    Real G = smith_masking(to_local(vertex.shading_frame, dir_in), roughness);
    Vector3 half_vector = normalize(dir_in + dir_out);
    Real cos_theta_h = dot(half_vector, vertex.shading_frame.n);
    Real D = GTR2(cos_theta_h, roughness);
    // (4 * cos_theta_v) is the Jacobian of the reflectiokn
    spec_prob *= (G * D) / (4 * n_dot_in);
    // For the diffuse lobe, we importance sample cos_theta_out
    diff_prob *= n_dot_out / c_PI;
    return spec_prob + diff_prob;
}
Real pdf_sample_bsdf_op::operator()(const RoughDielectric &bsdf) const {
    Vector3 n = vertex.shading_frame.n;
    Real n_dot_in = dot(dir_in, n);
    Real n_dot_out = dot(dir_out, n);
    // If we are going into the surface, then we use normal eta
    // (internal/external), otherwise we use external/internal.
    Real eta = n_dot_in > 0 ? bsdf.eta : 1 / bsdf.eta;
    assert(eta > 0);
    bool reflect = n_dot_in * n_dot_out > 0;
    Vector3 half_vector;
    if (reflect) {
        half_vector = normalize(dir_in + dir_out);
    } else {
        // "Generalized half-vector" from Walter et al.
        // See "Microfacet Models for Refraction through Rough Surfaces"
        half_vector = normalize(dir_in + dir_out * eta);
    }

    // Flip half-vector if it's below surface
    if (dot(half_vector, vertex.shading_frame.n) < 0) {
        half_vector = -half_vector;
    }

    Real roughness = eval(
        bsdf.roughness, vertex.uv, vertex.uv_screen_size, texture_pool);
    // Clamp roughness to avoid numerical issues.
    roughness = std::clamp(roughness, Real(0.01), Real(1));

    // We sample the visible normals, also we use F to determine
    // whether to sample reflection or refraction
    // so PDF ~ F * D * G_in for reflection, PDF ~ (1 - F) * D * G_in for refraction.
    Real h_dot_in = dot(half_vector, dir_in);
    Real F = fresnel_dielectric(h_dot_in, eta);
    Real D = GTR2(dot(half_vector, n), roughness);
    Real G = smith_masking(to_local(vertex.shading_frame, dir_in), roughness);

    if (reflect) {
        return (F * D * G) / (4 * fabs(n_dot_in));
    } else {
        Real h_dot_out = dot(half_vector, dir_out);
        Real sqrt_denom = h_dot_in + eta * h_dot_out;
        return (1 - F) * D * G * eta * eta * fabs(h_dot_out) / (sqrt_denom * sqrt_denom);
    }
}
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
struct sample_bsdf_op {
    std::optional<BSDFSampleRecord> operator()(const Lambertian &bsdf) const;
    std::optional<BSDFSampleRecord> operator()(const RoughPlastic &bsdf) const;
    std::optional<BSDFSampleRecord> operator()(const RoughDielectric &bsdf) const;

    const Vector3 &dir_in;
    const PathVertex &vertex;
    const TexturePool &texture_pool;
    const Vector2 &rnd_param_uv;
    const Real &rnd_param_w;
    const TransportDirection &dir;
};
std::optional<BSDFSampleRecord> sample_bsdf_op::operator()(const Lambertian &bsdf) const {
    // For Lambertian, we importance sample the cosine hemisphere domain.
    if (dot(vertex.shading_frame.n, dir_in) < 0) {
        // Incoming direction is below the surface.
        return {};
    }
    return BSDFSampleRecord{
        to_world(vertex.shading_frame, sample_cos_hemisphere(rnd_param_uv)),
        Real(0) /* eta */, Real(1) /* roughness */};
}
std::optional<BSDFSampleRecord>
        sample_bsdf_op::operator()(const RoughPlastic &bsdf) const {
    Vector3 n = vertex.shading_frame.n;
    Real cos_theta_in = dot(dir_in, n);
    if (cos_theta_in < 0) {
        // Incoming direction is below the surface.
        return {};
    }
    // We use the reflectance to choose between sampling the dielectric or diffuse layer.
    Spectrum Ks = eval(
        bsdf.specular_reflectance, vertex.uv, vertex.uv_screen_size, texture_pool);
    Spectrum Kd = eval(
        bsdf.diffuse_reflectance, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real lS = luminance(Ks), lR = luminance(Kd);
    if (lS + lR <= 0) {
        return {};
    }
    Real spec_prob = lS / (lS + lR);
    if (rnd_param_w < spec_prob) {
        // Sample from the specular lobe.

        // Convert the incoming direction to local coordinates
        Vector3 local_dir_in = to_local(vertex.shading_frame, dir_in);
        Real roughness = eval(
            bsdf.roughness, vertex.uv, vertex.uv_screen_size, texture_pool);
        // Clamp roughness to avoid numerical issues.
        roughness = std::clamp(roughness, Real(0.01), Real(1));
        Real alpha = roughness * roughness;
        Vector3 local_micro_normal =
            sample_visible_normals(local_dir_in, alpha, rnd_param_uv);
        
        // !!!! IMPORTANT !!!!
        // We can do the following two lines in two ways:
        // 1) first reflect the vector in local coordinates, then transform to world space
        // 2) first transform to world space, then reflect the vector
        // While they are mathemtically equivalent, it turns out that their
        // numerical properties are very different!!!
        // Since in other places of our code, we evaluate the half-vector in the world space,
        // if we do the reflection in the local coordinates, we suffer from floating point
        // precision loss of the world coordinates transformation. 
        // This turns out to be *extremely* crucial for very low roughness BRDFs.
        // You can try to play with this using the following commented out code.
        //
        // Vector3 reflected = 
        //     normalize(-local_dir_in + 2 * dot(local_dir_in, local_micro_normal) * local_micro_normal);
        // return to_world(vertex.shading_frame, reflected);

        // Transform the micro normal to world space
        Vector3 half_vector = to_world(vertex.shading_frame, local_micro_normal);
        // Reflect over the world space normal
        Vector3 reflected = normalize(-dir_in + 2 * dot(dir_in, half_vector) * half_vector);
        return BSDFSampleRecord{
            reflected,
            Real(0) /* eta */, roughness /* roughness */
        };
    } else {
        // Lambertian sampling
        return BSDFSampleRecord{
            to_world(vertex.shading_frame, sample_cos_hemisphere(rnd_param_uv)),
            Real(0) /* eta */, Real(1) /* roughness */};
    }
}
std::optional<BSDFSampleRecord>
        sample_bsdf_op::operator()(const RoughDielectric &bsdf) const {
    // If we are going into the surface, then we use normal eta
    // (internal/external), otherwise we use external/internal.
    Real eta = dot(dir_in, vertex.shading_frame.n) > 0 ? bsdf.eta : 1 / bsdf.eta;
    Real roughness = eval(
        bsdf.roughness, vertex.uv, vertex.uv_screen_size, texture_pool);
    // Clamp roughness to avoid numerical issues.
    roughness = std::clamp(roughness, Real(0.01), Real(1));
    // Sample a micro normal and transform it to world space -- this is our half-vector.
    Real alpha = roughness * roughness;
    Vector3 local_dir_in = to_local(vertex.shading_frame, dir_in);
    Vector3 local_micro_normal =
        sample_visible_normals(local_dir_in, alpha, rnd_param_uv);
    // See sample_bsdf_op::operator()(const RoughPlastic &bsdf)
    // for why we need to convert half vector to world coordinates here.
    Vector3 half_vector = to_world(vertex.shading_frame, local_micro_normal);
    // Now we need to decide whether to reflect or refract.
    // We do this using the Fresnel term.
    Real h_dot_in = dot(half_vector, dir_in);
    Real F = fresnel_dielectric(h_dot_in, eta);

    if (rnd_param_w <= F) {
        // Reflection
        Vector3 reflected = normalize(-dir_in + 2 * dot(dir_in, half_vector) * half_vector);
        // set eta to 0 since we are not transmitting
        return BSDFSampleRecord{reflected, Real(0) /* eta */, roughness};
    } else {
        // Refraction
        // https://en.wikipedia.org/wiki/Snell%27s_law#Vector_form
        // (note that our eta is eta2 / eta1, and l = -dir_in)
        Real h_dot_t_sq = 1 - (1 - h_dot_in * h_dot_in) / (eta * eta);
        if (h_dot_t_sq <= 0) {
            // Total internal reflection
            // This shouldn't really happen, as F will be 1 in this case.
            return {};
        }
        // flip half_vector if needed
        if (h_dot_in < 0) {
            half_vector = -half_vector;
        }
        Real h_dot_t = sqrt(h_dot_t_sq);
        Vector3 refracted = -dir_in / eta + (fabs(h_dot_in) / eta - h_dot_t) * half_vector;
        return BSDFSampleRecord{refracted, eta, roughness};
    }
}
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
struct get_texture_op {
    const TextureSpectrum& operator()(const Lambertian &bsdf) const;
    const TextureSpectrum& operator()(const RoughPlastic &bsdf) const;
    const TextureSpectrum& operator()(const RoughDielectric &bsdf) const;
};
const TextureSpectrum& get_texture_op::operator()(const Lambertian &bsdf) const {
    return bsdf.reflectance;
}
const TextureSpectrum& get_texture_op::operator()(const RoughPlastic &bsdf) const {
    return bsdf.diffuse_reflectance;
}
const TextureSpectrum& get_texture_op::operator()(const RoughDielectric &bsdf) const {
    return bsdf.specular_reflectance;
}
////////////////////////////////////////////////////////////////////////

Spectrum eval(const Material &material,
              const Vector3 &dir_light,
              const Vector3 &dir_view,
              const PathVertex &vertex,
              const TexturePool &texture_pool,
              TransportDirection dir) {
    return std::visit(eval_op{dir_light, dir_view, vertex, texture_pool, dir}, material);
}

std::optional<BSDFSampleRecord>
sample_bsdf(const Material &material,
            const Vector3 &dir_in,
            const PathVertex &vertex,
            const TexturePool &texture_pool,
            const Vector2 &rnd_param_uv,
            const Real &rnd_param_w,
            TransportDirection dir) {
    return std::visit(sample_bsdf_op{
        dir_in, vertex, texture_pool, rnd_param_uv, rnd_param_w, dir}, material);
}

Real pdf_sample_bsdf(const Material &material,
                     const Vector3 &dir_light,
                     const Vector3 &dir_view,
                     const PathVertex &vertex,
                     const TexturePool &texture_pool,
                     TransportDirection dir) {
    return std::visit(pdf_sample_bsdf_op{
        dir_light, dir_view, vertex, texture_pool, dir}, material);
}

const TextureSpectrum &get_texture(const Material &material) {
    return std::visit(get_texture_op{}, material);
}
