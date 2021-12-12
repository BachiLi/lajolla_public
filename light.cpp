#include "light.h"
#include "scene.h"

Real light_power::operator()(const DiffuseAreaLight &light) const {
    return luminance(light.intensity) *
           std::visit(surface_area{}, scene.shapes[light.shape_id]) *
           c_PI;
}

LightSample sample_point_on_light::operator()(const DiffuseAreaLight &light) const {
    const Shape &shape = scene.shapes[light.shape_id];
    ShapeSample shape_sample = std::visit(sample_point_on_shape{sample}, shape);
    return LightSample{shape_sample, light.intensity};
}

Real pdf_point_on_light::operator()(const DiffuseAreaLight &light) const {
    return std::visit(surface_area{}, scene.shapes[light.shape_id]);
}
