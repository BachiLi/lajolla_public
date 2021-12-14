#include "light.h"
#include "scene.h"
#include "spectrum.h"

////////////////////////////////////////////////////////////////////////////////
struct light_power_op {
    Real operator()(const DiffuseAreaLight &light) const;

    const Scene &scene;
};
Real light_power_op::operator()(const DiffuseAreaLight &light) const {
    return luminance(light.intensity) * surface_area(scene.shapes[light.shape_id]) * c_PI;
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
struct sample_point_on_light_op {
    LightSampleRecord operator()(const DiffuseAreaLight &light) const;

    const Vector2 &uv;
    const Real &w;
    const Scene &scene;
};
LightSampleRecord sample_point_on_light_op::operator()(const DiffuseAreaLight &light) const {
    const Shape &shape = scene.shapes[light.shape_id];
    PointAndNormal point_and_normal = sample_point_on_shape(shape, uv, w);
    return LightSampleRecord{uv, point_and_normal, light.intensity};
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
struct pdf_point_on_light_op {
    Real operator()(const DiffuseAreaLight &light) const;

    const PointAndNormal &point_on_light;
    const Scene &scene;
};
Real pdf_point_on_light_op::operator()(const DiffuseAreaLight &light) const {
    return pdf_point_on_shape(scene.shapes[light.shape_id]);
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
struct emission_op {
    Spectrum operator()(const DiffuseAreaLight &light) const;

    const Vector3 &view_dir;
    const PointAndNormal &point_on_light;
};
Spectrum emission_op::operator()(const DiffuseAreaLight &light) const {
    if (dot(point_on_light.normal, view_dir) <= 0) {
        return make_zero_spectrum();
    }
    return light.intensity;
}
////////////////////////////////////////////////////////////////////////////////

Real light_power(const Light &light, const Scene &scene) {
    return std::visit(light_power_op{scene}, light);
}

LightSampleRecord sample_point_on_light(const Light &light, const Vector2 &uv, Real w, const Scene &scene) {
    return std::visit(sample_point_on_light_op{uv, w, scene}, light);
}

Real pdf_point_on_light(const Light &light, const PointAndNormal &point_on_light, const Scene &scene) {
    return std::visit(pdf_point_on_light_op{point_on_light, scene}, light);
}

Spectrum emission(const Light &light, const Vector3 &view_dir, const PointAndNormal &point_on_light) {
    return std::visit(emission_op{view_dir, point_on_light}, light);
}
