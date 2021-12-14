#pragma once

#include "lajolla.h"
#include "table_dist.h"
#include "vector.h"
#include <embree3/rtcore.h>
#include <variant>
#include <vector>

struct PointAndNormal;

/// A Shape is a geometric entity that describes a surface. E.g., a sphere, a triangle mesh, a NURBS, etc.
/// For each shape, we also store an integer "material ID" that points to a material, and an integer
/// "area light ID" that points to a light source if the shape is an area light. area_lightID is set to -1
/// if the shape is not an area light.
struct ShapeBase {
    int material_id = -1;
    int area_light_id = -1;
};

struct Sphere : public ShapeBase {
    Vector3 position;
    Real radius;
};

struct TriangleMesh : public ShapeBase {
    /// TODO: make these portable to GPUs
    std::vector<Vector3> positions;
    std::vector<Vector3i> indices;
    std::vector<Vector3> normals;
    std::vector<Vector2> uvs;
    /// Below are used only when the mesh is associated with an area light
    Real total_area;
    TableDist1D triangle_sampler;
};

// To add more shapes, first create a struct for the shape, add it to the variant below,
// then implement all the relevant functions below.
using Shape = std::variant<Sphere, TriangleMesh>;

/// Add the shape to an Embree scene.
uint32_t register_embree(const Shape &shape, const RTCDevice &device, const RTCScene &scene);
/// Sample a point on the surface
PointAndNormal sample_point_on_shape(const Shape &shape, const Vector2 &uv, Real w);
/// PDF of the operation above
Real pdf_point_on_shape(const Shape &shape);
Real surface_area(const Shape &shape);
void init_sampling_dist(Shape &shape);


inline void set_material_id(Shape &shape, int material_id) {
    std::visit([&](auto &s) { s.material_id = material_id; }, shape);
}
inline void set_area_light_id(Shape &shape, int area_light_id) {
    std::visit([&](auto &s) { s.area_light_id = area_light_id; }, shape);
}
inline int get_material_id(const Shape &shape) {
    return std::visit([&](const auto &s) { return s.material_id; }, shape);
}
inline int get_area_light_id(const Shape &shape) {
    return std::visit([&](const auto &s) { return s.area_light_id; }, shape);
}
inline bool is_light(const Shape &shape) {
    return get_area_light_id(shape) >= 0;
}
