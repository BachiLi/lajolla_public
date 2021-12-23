#include "../microfacet.h"

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
    Real G = smith_masking_gtr2(to_local(vertex.shading_frame, dir_in), roughness) *
             smith_masking_gtr2(to_local(vertex.shading_frame, dir_out), roughness);

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
    Real G = smith_masking_gtr2(to_local(vertex.shading_frame, dir_in), roughness);
    Vector3 half_vector = normalize(dir_in + dir_out);
    Real cos_theta_h = dot(half_vector, vertex.shading_frame.n);
    Real D = GTR2(cos_theta_h, roughness);
    // (4 * cos_theta_v) is the Jacobian of the reflectiokn
    spec_prob *= (G * D) / (4 * n_dot_in);
    // For the diffuse lobe, we importance sample cos_theta_out
    diff_prob *= n_dot_out / c_PI;
    return spec_prob + diff_prob;
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

const TextureSpectrum& get_texture_op::operator()(const RoughPlastic &bsdf) const {
    return bsdf.diffuse_reflectance;
}
