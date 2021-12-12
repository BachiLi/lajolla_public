#pragma once

#include "lajolla.h"
#include "vector.h"
#include <variant>

struct Intersection;

struct Lambertian {
	Vector3 reflectance;
};

// To add more materials, first create a struct for the material, then overload the () operators for all the
// functors below.
using Material = std::variant<Lambertian>;

/// Given incoming direction and outgoing direction of lights,
/// outputs the energy density at a point.
struct eval_material {
    Vector3 operator()(const Lambertian &lambertian) const;

    Vector3 w_light, w_view;
    const Intersection &isect;
};
