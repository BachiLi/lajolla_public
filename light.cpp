#include "light.h"
#include "scene.h"
#include "spectrum.h"

struct light_power_op {
    Real operator()(const DiffuseAreaLight &light) const;

    const Scene &scene;
};

Real light_power_op::operator()(const DiffuseAreaLight &light) const {
    return luminance(light.intensity) * surface_area(scene.shapes[light.shape_id]) * c_PI;
}

struct sample_point_on_light_op {
    LightSampleRecord operator()(const DiffuseAreaLight &light) const;

    const Vector2 &uv;
    const Real &w;
    const Scene &scene;
};

LightSampleRecord sample_point_on_light_op::operator()(const DiffuseAreaLight &light) const {
    const Shape &shape = scene.shapes[light.shape_id];
    ShapeSampleRecord shape_sample = sample_point_on_shape(shape, uv, w);
    return LightSampleRecord{uv, shape_sample, light.intensity};
}

struct pdf_point_on_light_op {
    Real operator()(const DiffuseAreaLight &light) const;

    const LightSampleRecord &record;
    const Scene &scene;
};

Real pdf_point_on_light_op::operator()(const DiffuseAreaLight &light) const {
    return pdf_point_on_shape(scene.shapes[light.shape_id]);
}

Real light_power(const Light &light, const Scene &scene) {
    return std::visit(light_power_op{scene}, light);
}

LightSampleRecord sample_point_on_light(const Light &light, const Vector2 &uv, Real w, const Scene &scene) {
    return std::visit(sample_point_on_light_op{uv, w, scene}, light);
}

Real pdf_point_on_light(const Light &light, const LightSampleRecord &record, const Scene &scene) {
    return std::visit(pdf_point_on_light_op{record, scene}, light);
}
