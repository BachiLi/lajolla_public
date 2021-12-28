Real light_power_op::operator()(const DiffuseAreaLight &light) const {
    return luminance(light.intensity) * surface_area(scene.shapes[light.shape_id]) * c_PI;
}

PointAndNormal sample_point_on_light_op::operator()(const DiffuseAreaLight &light) const {
    const Shape &shape = scene.shapes[light.shape_id];
    return sample_point_on_shape(shape, ref_point, rnd_param_uv, rnd_param_w);
}

Real pdf_point_on_light_op::operator()(const DiffuseAreaLight &light) const {
    return pdf_point_on_shape(
        scene.shapes[light.shape_id], point_on_light, ref_point);
}

Spectrum emission_op::operator()(const DiffuseAreaLight &light) const {
    if (dot(point_on_light.normal, view_dir) <= 0) {
        return make_zero_spectrum();
    }
    return light.intensity;
}

void init_sampling_dist_op::operator()(DiffuseAreaLight &light) const {
}
