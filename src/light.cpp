#include "light.h"
#include "scene.h"
#include "spectrum.h"
#include "transform.h"

struct light_power_op {
    Real operator()(const DiffuseAreaLight &light) const;
    Real operator()(const Envmap &light) const;

    const Scene &scene;
};

struct sample_point_on_light_op {
    PointAndNormal operator()(const DiffuseAreaLight &light) const;
    PointAndNormal operator()(const Envmap &light) const;

    const Vector3 &ref_point;
    const Vector2 &rnd_param_uv;
    const Real &rnd_param_w;
    const Scene &scene;
};

struct pdf_point_on_light_op {
    Real operator()(const DiffuseAreaLight &light) const;
    Real operator()(const Envmap &light) const;

    const PointAndNormal &point_on_light;
    const Vector3 &ref_point;
    const Scene &scene;
};

struct emission_op {
    Spectrum operator()(const DiffuseAreaLight &light) const;
    Spectrum operator()(const Envmap &light) const;

    const Vector3 &view_dir;
    const PointAndNormal &point_on_light;
    Real view_footprint;
    const Scene &scene;
};

struct init_sampling_dist_op {
    void operator()(DiffuseAreaLight &light) const;
    void operator()(Envmap &light) const;

    const Scene &scene;
};

#include "lights/diffuse_area_light.inl"
#include "lights/envmap.inl"

Real light_power(const Light &light, const Scene &scene) {
    return std::visit(light_power_op{scene}, light);
}

PointAndNormal sample_point_on_light(const Light &light,
                                     const Vector3 &ref_point,
                                     const Vector2 &rnd_param_uv,
                                     Real rnd_param_w,
                                     const Scene &scene) {
    return std::visit(sample_point_on_light_op{ref_point, rnd_param_uv, rnd_param_w, scene}, light);
}

Real pdf_point_on_light(const Light &light,
                        const PointAndNormal &point_on_light,
                        const Vector3 &ref_point,
                        const Scene &scene) {
    return std::visit(pdf_point_on_light_op{point_on_light, ref_point, scene}, light);
}

Spectrum emission(const Light &light,
                  const Vector3 &view_dir,
                  Real view_footprint,
                  const PointAndNormal &point_on_light,
                  const Scene &scene) {
    return std::visit(emission_op{view_dir, point_on_light, view_footprint, scene}, light);
}

void init_sampling_dist(Light &light, const Scene &scene) {
    return std::visit(init_sampling_dist_op{scene}, light);
}
