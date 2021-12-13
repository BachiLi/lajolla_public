#pragma once

#include "lajolla.h"
#include "shape.h"
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
    ShapeSampleRecord shape_sample_rec;
    Vector3 intensity;
};

Real light_power(const Light &light, const Scene &scene);
LightSampleRecord sample_point_on_light(const Light &light, const Vector2 &uv, Real w, const Scene &scene);
Real pdf_point_on_light(const Light &light, const LightSampleRecord &record, const Scene &scene);
