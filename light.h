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

// To add more lights, first create a struct for the light, add it to the variant type below,
// then implement all the relevant function below with the Light type.
using Light = std::variant<DiffuseAreaLight>;

struct LightSampleRecord {
    PointAndNormal point_on_light;
    Vector3 radiance;
};

/// Computes the total power the light emit to all positions and directions.
/// Useful for sampling.
Real light_power(const Light &light, const Scene &scene);

/// Given some random numbers, sample a point on the light source.
/// rnd_param_w is usually used for choosing a discrete element e.g., choosing a triangle in a mesh light.
/// rnd_param_uv is usually used for picking a point on that element.
LightSampleRecord sample_point_on_light(const Light &light, 
                                        const Vector2 &rnd_param_uv,
                                        Real rnd_param_w,
                                        const Scene &scene);

/// Given a point on the light source, compute the sampling density for the function above.
Real pdf_point_on_light(const Light &light, const PointAndNormal &point_on_light, const Scene &scene);

/// Given a viewing direction pointing outwards from the light, and a point on the light,
/// compute the emission of the light.
Spectrum emission(const Light &light, const Vector3 &view_dir, const PointAndNormal &point_on_light);
