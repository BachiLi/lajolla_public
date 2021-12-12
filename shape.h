#pragma once

#include "lajolla.h"
#include "vector.h"
#include <embree3/rtcore.h>
#include <variant>

/// A Shape is a geometric entity that describes a surface. E.g., a sphere, a triangle mesh, a NURBS, etc.
/// For each shape, we also store an integer "material ID" that points to a material, and an integer
/// "area light ID" that points to a light source if the shape is an area light. area_lightID is set to -1
/// if the shape is not an area light.
struct ShapeBase {
    int materialID;
    int area_lightID;
};

struct Sphere : public ShapeBase {
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

struct ShapeSample {
    Vector3 position;
    Vector3 geometry_normal;
};

struct sample_point_on_shape {
    ShapeSample operator()(const Sphere &sphere) const;

    Vector2 sample;
};

struct surface_area {
    Real operator()(const Sphere &sphere) const;
};
