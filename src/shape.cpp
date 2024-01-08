#include "shape.h"
#include "intersection.h"
#include "point_and_normal.h"
#include "ray.h"
#include <embree4/rtcore.h>

struct register_embree_op {
    uint32_t operator()(const Sphere &sphere) const;
    uint32_t operator()(const TriangleMesh &mesh) const;

    const RTCDevice &device;
    const RTCScene &scene;
};

struct sample_point_on_shape_op {
    PointAndNormal operator()(const Sphere &sphere) const;
    PointAndNormal operator()(const TriangleMesh &mesh) const;

    const Vector3 &ref_point;
    const Vector2 &uv; // for selecting a point on a 2D surface
    const Real &w; // for selecting triangles
};

struct surface_area_op {
    Real operator()(const Sphere &sphere) const;
    Real operator()(const TriangleMesh &mesh) const;
};

struct pdf_point_on_shape_op {
    Real operator()(const Sphere &sphere) const;
    Real operator()(const TriangleMesh &mesh) const;

    const PointAndNormal &point_on_shape;
    const Vector3 &ref_point;
};

struct init_sampling_dist_op {
    void operator()(Sphere &sphere) const;
    void operator()(TriangleMesh &mesh) const;
};

struct compute_shading_info_op {
    ShadingInfo operator()(const Sphere &sphere) const;
    ShadingInfo operator()(const TriangleMesh &mesh) const;

    const PathVertex &vertex;
};

#include "shapes/sphere.inl"
#include "shapes/triangle_mesh.inl"

uint32_t register_embree(const Shape &shape, const RTCDevice &device, const RTCScene &scene) {
    return std::visit(register_embree_op{device, scene}, shape);
}

PointAndNormal sample_point_on_shape(const Shape &shape,
                                     const Vector3 &ref_point,
                                     const Vector2 &uv,
                                     Real w) {
    return std::visit(sample_point_on_shape_op{ref_point, uv, w}, shape);
}

Real pdf_point_on_shape(const Shape &shape,
                        const PointAndNormal &point_on_shape,
                        const Vector3 &ref_point) {
    return std::visit(pdf_point_on_shape_op{point_on_shape, ref_point}, shape);
}

Real surface_area(const Shape &shape) {
    return std::visit(surface_area_op{}, shape);
}

void init_sampling_dist(Shape &shape) {
    return std::visit(init_sampling_dist_op{}, shape);
}

ShadingInfo compute_shading_info(const Shape &shape, const PathVertex &vertex) {
    return std::visit(compute_shading_info_op{vertex}, shape);
}
