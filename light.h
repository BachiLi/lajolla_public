#pragma once

#include "lajolla.h"
#include "point_and_normal.h"
#include "shape.h"
#include "spectrum.h"
#include "vector.h"
#include <variant>

struct Scene;

struct DiffuseAreaLight {
    int shape_id;
    Vector3 intensity;
};

// To add more lights, first create a struct for the light, then overload the () operators for all the
// functors below.
using Light = std::variant<DiffuseAreaLight>;

struct LightSampleRecord {
    Vector2 uv;
    PointAndNormal point_on_light;
    Vector3 radiance;
};

Real light_power(const Light &light, const Scene &scene);
LightSampleRecord sample_point_on_light(const Light &light, const Vector2 &uv, Real w, const Scene &scene);
Real pdf_point_on_light(const Light &light, const PointAndNormal &point_on_light, const Scene &scene);
Spectrum emission(const Light &light, const Vector3 &view_dir, const PointAndNormal &point_on_light);
