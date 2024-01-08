#pragma once

#include "lajolla.h"
#include "frame.h"
#include "table_dist.h"
#include "vector.h"
#include <embree4/rtcore.h>
#include <variant>
#include <vector>

struct PointAndNormal;
struct PathVertex;

struct ShadingInfo {
    Vector2 uv; // UV coordinates for texture mapping
    Frame shading_frame; // the coordinate basis for shading
    Real mean_curvature; // 0.5 * (dN/du + dN/dv)
    // Stores min(length(dp/du), length(dp/dv)), for ray differentials.
    Real inv_uv_size;
};

/// A Shape is a geometric entity that describes a surface. E.g., a sphere, a triangle mesh, a NURBS, etc.
/// For each shape, we also store an integer "material ID" that points to a material, and an integer
/// "area light ID" that points to a light source if the shape is an area light. area_lightID is set to -1
/// if the shape is not an area light.
struct ShapeBase {
    int material_id = -1;
    int area_light_id = -1;

    int interior_medium_id = -1;
    int exterior_medium_id = -1;
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
    /// For sampling a triangle based on its area
    TableDist1D triangle_sampler;
};

// To add more shapes, first create a struct for the shape, add it to the variant below,
// then implement all the relevant functions below.
using Shape = std::variant<Sphere, TriangleMesh>;

/// Add the shape to an Embree scene.
uint32_t register_embree(const Shape &shape, const RTCDevice &device, const RTCScene &scene);

/// Sample a point on the surface given a reference point.
/// uv & w are uniform random numbers.
PointAndNormal sample_point_on_shape(const Shape &shape,
                                     const Vector3 &ref_point,
                                     const Vector2 &uv,
                                     Real w);

/// Probability density of the operation above
Real pdf_point_on_shape(const Shape &shape,
                        const PointAndNormal &point_on_shape,
                        const Vector3 &ref_point);

/// Useful for sampling.
Real surface_area(const Shape &shape);

/// Some shapes require storing sampling data structures inside. This function initialize them.
void init_sampling_dist(Shape &shape);

/// Embree doesn't calculate some shading information for us. We have to do it ourselves.
ShadingInfo compute_shading_info(const Shape &shape, const PathVertex &vertex);

inline void set_material_id(Shape &shape, int material_id) {
    std::visit([&](auto &s) { s.material_id = material_id; }, shape);
}
inline void set_area_light_id(Shape &shape, int area_light_id) {
    std::visit([&](auto &s) { s.area_light_id = area_light_id; }, shape);
}
inline void set_interior_medium_id(Shape &shape, int interior_medium_id) {
    std::visit([&](auto &s) { s.interior_medium_id = interior_medium_id; }, shape);
}
inline void set_exterior_medium_id(Shape &shape, int exterior_medium_id) {
    std::visit([&](auto &s) { s.exterior_medium_id = exterior_medium_id; }, shape);
}
inline int get_material_id(const Shape &shape) {
    return std::visit([&](const auto &s) { return s.material_id; }, shape);
}
inline int get_area_light_id(const Shape &shape) {
    return std::visit([&](const auto &s) { return s.area_light_id; }, shape);
}
inline int get_interior_medium_id(const Shape &shape) {
    return std::visit([&](const auto &s) { return s.interior_medium_id; }, shape);
}
inline int get_exterior_medium_id(const Shape &shape) {
    return std::visit([&](const auto &s) { return s.exterior_medium_id; }, shape);
}
inline bool is_light(const Shape &shape) {
    return get_area_light_id(shape) >= 0;
}
