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

struct ShapeSample {
    Vector3 position;
    Vector3 geometry_normal;
};

/// Add the shape to an Embree scene.
uint32_t register_embree(const Shape &shape, const RTCDevice &device, const RTCScene &scene);
ShapeSample sample_point_on_shape(const Shape &shape, const Vector2 &sample);
Real surface_area(const Shape &shape);
