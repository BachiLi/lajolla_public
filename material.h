#pragma once

#include "lajolla.h"
#include "spectrum.h"
#include "vector.h"
#include <optional>
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

/// We allow non-reciprocal BRDFs, so it's important
/// to distinguish which direction we are tracing the rays.
enum class TransportDirection {
    TO_LIGHT,
    TO_VIEW
};

/// Given incoming direction pointing outwards of the intersection point,
/// samples an outgoing direction.
/// If dir == TO_LIGHT, incoming direction is dir_view and 
/// we're sampling for dir_light. Vice versa.
std::optional<Vector3> sample_bsdf(const Material &material,
                                   const Vector3 &dir_in,
                                   const Intersection &isect,
                                   const Vector2 &rnd_param,
                                   TransportDirection dir = TransportDirection::TO_LIGHT);

/// Given incoming direction and outgoing direction of lights,
/// both pointing outwards of the intersection point,
/// outputs the probability density of sampling.
/// If dir == TO_LIGHT, incoming direction is dir_view and 
/// we're sampling for dir_light. Vice versa.
Real pdf_sample_bsdf(const Material &material,
                     const Vector3 &dir_light,
                     const Vector3 &dir_view,
                     const Intersection &isect,
                     TransportDirection dir = TransportDirection::TO_LIGHT);
