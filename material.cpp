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

    const Vector3 &dir_light;
    const Vector3 &dir_view;
    const PathVertex &vertex;
    const TexturePool &texture_pool;
};
Spectrum eval_op::operator()(const Lambertian &bsdf) const {
    if (dot(dir_view, vertex.shading_frame.n) < 0) {
        // Viewing direction is below the surface.
        return make_zero_spectrum();
    }
    return fmax(dot(dir_light, vertex.shading_frame.n), Real(0)) * 
           eval(bsdf.reflectance, vertex.uv, vertex.uv_screen_size, texture_pool) / c_PI;
}
Spectrum eval_op::operator()(const RoughPlastic &bsdf) const {
    Real cos_theta_l = dot(dir_light, vertex.shading_frame.n);
    Real cos_theta_v = dot(dir_view, vertex.shading_frame.n);
    if (cos_theta_l <= 0 || cos_theta_v <= 0) {
        // No light on the other side.
        return make_zero_spectrum();
    }
    Spectrum R = eval(
        bsdf.diffuse_reflectance, vertex.uv, vertex.uv_screen_size, texture_pool);
    Spectrum S = eval(
        bsdf.specular_reflectance, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real roughness = eval(
        bsdf.roughness, vertex.uv, vertex.uv_screen_size, texture_pool);
    // Clamp roughness to avoid numerical issues.
    roughness = std::clamp(roughness, Real(0.01), Real(1));
    // We first account for the dielectric layer.

    // The light first hits the outer layer. Fresnel equation determines
    // how much light goes through, and how much light is reflected for each wavelength.
    Real F_l = fresnel_dielectric(bsdf.eta, cos_theta_l); // F_l is the reflection percentage.
    Vector3 half_vector = normalize(dir_light + dir_view);
    Real cos_theta_h = dot(half_vector, vertex.shading_frame.n);
    Real D = GTR2(cos_theta_h, roughness); // "Generalized Trowbridge Reitz", GTR2 is equivalent to GGX.
    Real G = smith_masking(to_local(vertex.shading_frame, dir_view), roughness) *
             smith_masking(to_local(vertex.shading_frame, dir_light), roughness);

    Spectrum spec_contrib = S * (G * F_l * D) / (4 * cos_theta_l * cos_theta_v);

    // Next we account for the diffuse layer.
    // In order to reflect from the diffuse layer,
    // the photon needs to bounce through the dielectric layers twice.
    // The transmittance is computed by 1 - fresnel.
    Real F_v = fresnel_dielectric(bsdf.eta, cos_theta_v);
    // Multiplying with Fresnels leads to an overly dark appearance at the 
    // object boundaries. Disney BRDF proposes a fix to this -- we will implement this in problem set 1.
    Spectrum diffuse_contrib = R * (Real(1) - F_l) * (Real(1) - F_v) / c_PI;

    return (spec_contrib + diffuse_contrib) * cos_theta_l;
}
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
struct pdf_sample_bsdf_op {
    Real operator()(const Lambertian &bsdf) const;
    Real operator()(const RoughPlastic &bsdf) const;

    const Vector3 &dir_light;
    const Vector3 &dir_view;
    const PathVertex &vertex;
    const TexturePool &texture_pool;
    const TransportDirection &dir;
};
Real pdf_sample_bsdf_op::operator()(const Lambertian &bsdf) const {
    // For Lambertian, we importance sample the cosine hemisphere domain.
    Vector3 n = vertex.shading_frame.n;
    Real cos_theta_l = dot(dir_light, n);
    Real cos_theta_v = dot(dir_view, n);
    if (cos_theta_l <= 0 || cos_theta_v <= 0) {
        // No light on the other side.
        return 0;
    }
    if (dir == TransportDirection::TO_LIGHT) {
        return fmax(cos_theta_l, Real(0)) / c_PI;
    } else {
        return fmax(cos_theta_v, Real(0)) / c_PI;
    }
}
Real pdf_sample_bsdf_op::operator()(const RoughPlastic &bsdf) const {
    Real cos_theta_l = dot(dir_light, vertex.shading_frame.n);
    Real cos_theta_v = dot(dir_view, vertex.shading_frame.n);
    if (cos_theta_l <= 0 || cos_theta_v <= 0) {
        // No light on the other side.
        return 0;
    }
    const Vector3 &dir_in = dir == TransportDirection::TO_LIGHT ? dir_view : dir_light;
    Real cos_theta_out = dir == TransportDirection::TO_LIGHT ? cos_theta_l : cos_theta_v;
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
    Vector3 half_vector = normalize(dir_light + dir_view);
    Real cos_theta_h = dot(half_vector, vertex.shading_frame.n);
    Real D = GTR2(cos_theta_h, roughness);
    // (4 * cos_theta_v) is the Jacobian of the reflectiokn
    spec_prob *= (G * D) / (4 * cos_theta_v);
    // For the diffuse lobe, we importance sample cos_theta_out
    diff_prob *= cos_theta_out / c_PI;
    return spec_prob + diff_prob;
}
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
struct sample_bsdf_op {
    std::optional<Vector3> operator()(const Lambertian &bsdf) const;
    std::optional<Vector3> operator()(const RoughPlastic &bsdf) const;

    const Vector3 &dir_in;
    const PathVertex &vertex;
    const TexturePool &texture_pool;
    const Vector2 &rnd_param;
    const TransportDirection &dir;
};
std::optional<Vector3> sample_bsdf_op::operator()(const Lambertian &bsdf) const {
    // For Lambertian, we importance sample the cosine hemisphere domain.
    if (dot(vertex.shading_frame.n, dir_in) < 0) {
        // Incoming direction is below the surface.
        return {};
    }
    return to_world(vertex.shading_frame, sample_cos_hemisphere(rnd_param));
}
std::optional<Vector3> sample_bsdf_op::operator()(const RoughPlastic &bsdf) const {
    Vector3 n = vertex.shading_frame.n;
    Real cos_theta_in = dot(dir_in, n);
    if (cos_theta_in < 0) {
        // Incoming direction is below the surface.
        return {};
    }
    // We use the reflectance to choose between sampling the dielectric or diffuse layer.
    Spectrum S = eval(
        bsdf.specular_reflectance, vertex.uv, vertex.uv_screen_size, texture_pool);
    Spectrum R = eval(
        bsdf.diffuse_reflectance, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real lS = luminance(S), lR = luminance(R);
    if (lS + lR <= 0) {
        return {};
    }
    Real spec_prob = lS / (lS + lR);
    Vector2 u = rnd_param;
    if (u.x < spec_prob) {
        // Remap u from [0, spec_prob] to [0, 1]
        u.x /= spec_prob;
        // Sample from the specular lobe.

        // See "Sampling the GGX Distribution of Visible Normals", Heitz, 2018.
        // https://jcgt.org/published/0007/04/01/

        // Convert the incoming direction to local coordinates
        // This is the "ellipsodial configuration" in Heitz's paper
        Vector3 ellip_dir_in = to_local(vertex.shading_frame, dir_in);

        Real roughness = eval(
            bsdf.roughness, vertex.uv, vertex.uv_screen_size, texture_pool);
        // Clamp roughness to avoid numerical issues.
        roughness = std::clamp(roughness, Real(0.01), Real(1));

        Real alpha = roughness * roughness;

        // Transform the incoming direction to the "hemisphere configuration".
        Vector3 hemi_dir_in = normalize(
            Vector3{alpha * ellip_dir_in.x, alpha * ellip_dir_in.y, ellip_dir_in.z});

        // Parameterization of the projected area of a hemisphere.
        // First, sample a disk.
        Real r = sqrt(u.x);
        Real phi = 2 * c_PI * u.y;
        Real t1 = r * cos(phi);
        Real t2 = r * sin(phi);
        // Vertically scale the position of a sample to account for the projection.
        Real s = (1 + hemi_dir_in.z) / 2;
        t2 = (1 - s) * sqrt(1 - t1 * t1) + s * t2;
        // Point in the disk space
        Vector3 disk_N{t1, t2, sqrt(max(Real(0), 1 - t1*t1 - t2*t2))};

        // Reprojection onto hemisphere -- we get our sampled normal in hemisphere space.
        // Frame hemi_frame(hemi_dir_in);
        // Vector3 hemi_N = to_world(hemi_frame, disk_N);
        Vector3 T1 = (hemi_dir_in.z < Real(0.99999)) ? 
            normalize(cross(Vector3{0, 0, 1}, hemi_dir_in)) : Vector3{1, 0, 0};
        Vector3 T2 = cross(hemi_dir_in, T1);
        Vector3 hemi_N = normalize(disk_N.x * T1 + disk_N.y * T2 + disk_N.z * hemi_dir_in);

        // Transforming the normal back to the ellipsoid configuration
        Vector3 ellip_N = normalize(
            Vector3{alpha * hemi_N.x, alpha * hemi_N.y, max(Real(0), hemi_N.z)});
        
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
        // Vector3 reflected = normalize(-ellip_dir_in + 2 * dot(ellip_dir_in, ellip_N) * ellip_N);
        // return to_world(vertex.shading_frame, reflected);

        // Transform the normal to world space
        Vector3 world_N = to_world(vertex.shading_frame, ellip_N);

        // Reflect over the world space normal
        Vector3 reflected = normalize(-dir_in + 2 * dot(dir_in, world_N) * world_N);
        return reflected;
    } else {
        // Remap u from [spec_prob, 1] to [0, 1]
        u.x = (u.x - spec_prob) / (1 - spec_prob);

        // Lambertian sampling
        return to_world(vertex.shading_frame, sample_cos_hemisphere(u));
    }
}
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
struct get_roughness_op {
    Real operator()(const Lambertian &bsdf) const;
    Real operator()(const RoughPlastic &bsdf) const;

    const PathVertex &vertex;
    const TexturePool &texture_pool;
};
Real get_roughness_op::operator()(const Lambertian &bsdf) const {
    return Real(1);
}
Real get_roughness_op::operator()(const RoughPlastic &bsdf) const {
    return eval(bsdf.roughness, vertex.uv, vertex.uv_screen_size, texture_pool);
}
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
struct get_texture_op {
    const TextureSpectrum& operator()(const Lambertian &bsdf) const;
    const TextureSpectrum& operator()(const RoughPlastic &bsdf) const;
};
const TextureSpectrum& get_texture_op::operator()(const Lambertian &bsdf) const {
    return bsdf.reflectance;
}
const TextureSpectrum& get_texture_op::operator()(const RoughPlastic &bsdf) const {
    return bsdf.diffuse_reflectance;
}
////////////////////////////////////////////////////////////////////////

Spectrum eval(const Material &material,
              const Vector3 &dir_light,
              const Vector3 &dir_view,
              const PathVertex &vertex,
              const TexturePool &texture_pool) {
    return std::visit(eval_op{dir_light, dir_view, vertex, texture_pool}, material);
}

std::optional<Vector3> sample_bsdf(const Material &material,
                                   const Vector3 &dir_in,
                                   const PathVertex &vertex,
                                   const TexturePool &texture_pool,
                                   const Vector2 &rnd_param,
                                   TransportDirection dir) {
    return std::visit(sample_bsdf_op{
        dir_in, vertex, texture_pool, rnd_param, dir}, material);
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

Real get_roughness(const Material &material,
                   const PathVertex &vertex,
                   const TexturePool &texture_pool) {
    return std::visit(get_roughness_op{vertex, texture_pool}, material);
}

const TextureSpectrum &get_texture(const Material &material) {
    return std::visit(get_texture_op{}, material);
}
