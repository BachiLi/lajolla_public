#pragma once

#include "lajolla.h"
#include "vector.h"
#include <embree3/rtcore.h>
#include <variant>

/// A Shape is a geometric entity that describes a surface. E.g., a sphere, a triangle mesh, a NURBS, etc.

struct Sphere {
    Vector3 position;
    Real radius;
};

// To add more shapes, first create a struct for the shape, then overload the () operators for all the
// functors below.
using Shape = std::variant<Sphere>;

/// Add the shape to an Embree scene.
struct register_embree {
    uint32_t operator()(const Sphere &sphere) const;

    RTCDevice device;
    RTCScene scene;
};
