Spectrum eval_op::operator()(const Lambertian &bsdf) const {
    Vector3 n = vertex.shading_frame.n;
    Real n_dot_in = dot(dir_in, n);
    Real n_dot_out = dot(dir_out, n);
    if (n_dot_in <= 0 || n_dot_out <= 0) {
        // No light on the other side.
        return make_zero_spectrum();
    }

    return n_dot_out * 
           eval(bsdf.reflectance, vertex.uv, vertex.uv_screen_size, texture_pool) / c_PI;
}

Real pdf_sample_bsdf_op::operator()(const Lambertian &bsdf) const {
    // For Lambertian, we importance sample the cosine hemisphere domain.
    Vector3 n = vertex.shading_frame.n;
    Real n_dot_in = dot(dir_in, n);
    Real n_dot_out = dot(dir_out, n);
    if (n_dot_in <= 0 || n_dot_out <= 0) {
        // No light on the other side.
        return 0;
    }

    return n_dot_out / c_PI;
}

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

const TextureSpectrum& get_texture_op::operator()(const Lambertian &bsdf) const {
    return bsdf.reflectance;
}

