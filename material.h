#pragma once

#include "lajolla.h"
#include "spectrum.h"
#include "vector.h"
#include <variant>

struct Intersection;

struct Lambertian {
	Spectrum reflectance;
};

// To add more materials, first create a struct for the material, then overload the () operators for all the
// functors below.
using Material = std::variant<Lambertian>;

/// Given incoming direction and outgoing direction of lights,
/// both pointing outwards of the intersection point,
/// outputs the energy density at a point.
Spectrum eval(const Material &material,
              const Vector3 &dir_light,
              const Vector3 &dir_view,
              const Intersection &isect);
