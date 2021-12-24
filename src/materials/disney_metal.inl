#include "../microfacet.h"

Spectrum eval_op::operator()(const DisneyMetal &bsdf) const {
    Real n_dot_in = dot(dir_in, vertex.shading_frame.n);
    Real n_dot_out = dot(dir_out, vertex.shading_frame.n);
    if (n_dot_in <= 0 || n_dot_out <= 0) {
        // No light on the other side.
        return make_zero_spectrum();
    }
    // Homework 1: implement this!

    return make_zero_spectrum();
}

Real pdf_sample_bsdf_op::operator()(const DisneyMetal &bsdf) const {
    Vector3 n = vertex.shading_frame.n;
    Real n_dot_in = dot(dir_in, n);
    Real n_dot_out = dot(dir_out, n);
    if (n_dot_in <= 0 || n_dot_out <= 0) {
        // No light on the other side.
        return 0;
    }
    // Homework 1: implement this!

    return 0;
}

std::optional<BSDFSampleRecord>
        sample_bsdf_op::operator()(const DisneyMetal &bsdf) const {
    Vector3 n = vertex.shading_frame.n;
    Real cos_theta_in = dot(dir_in, n);
    if (cos_theta_in < 0) {
        // Incoming direction is below the surface.
        return {};
    }
    // Homework 1: implement this!

    return {};
}

const TextureSpectrum& get_texture_op::operator()(const DisneyMetal &bsdf) const {
    return bsdf.base_color;
}
