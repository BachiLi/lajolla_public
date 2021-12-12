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

struct LightSample {
    ShapeSample shape_sample;
    Vector3 intensity;
};

struct light_power {
    Real operator()(const DiffuseAreaLight &light) const;

    const Scene &scene;
};

struct sample_point_on_light {
    LightSample operator()(const DiffuseAreaLight &light) const;

    Vector2 sample;
    const Scene &scene;
};

struct pdf_point_on_light {
    Real operator()(const DiffuseAreaLight &light) const;

    const Scene &scene;
};
