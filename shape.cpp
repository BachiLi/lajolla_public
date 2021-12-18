#include "shape.h"
#include "intersection.h"
#include "point_and_normal.h"
#include "ray.h"
#include <embree3/rtcore.h>

void sphere_bounds_func(const struct RTCBoundsFunctionArguments* args) {
    const Sphere *sphere = (const Sphere*) args->geometryUserPtr;
    RTCBounds* bounds_o = args->bounds_o;
    bounds_o->lower_x = sphere->position.x - sphere->radius;
    bounds_o->lower_y = sphere->position.y - sphere->radius;
    bounds_o->lower_z = sphere->position.z - sphere->radius;
    bounds_o->upper_x = sphere->position.x + sphere->radius;
    bounds_o->upper_y = sphere->position.y + sphere->radius;
    bounds_o->upper_z = sphere->position.z + sphere->radius;
}

/// Numerically stable quadratic equation solver at^2 + bt + c = 0
/// See https://people.csail.mit.edu/bkph/articles/Quadratics.pdf
/// returns false when it can't find solutions.
bool solve_quadratic(Real a, Real b, Real c, Real *t0, Real *t1) {
    // Degenerated case
    if (a == 0) {
        if (b == 0) {
            return false;
        }
        *t0 = *t1 = -c / b;
        return true;
    }

    Real discriminant = b * b - 4 * a * c;
    if (discriminant < 0) {
        return false;
    }
    Real root_discriminant = sqrt(discriminant);
    if (b >= 0) {
        *t0 = (- b - root_discriminant) / (2 * a);
        *t1 = 2 * c / (- b - root_discriminant);
    } else {
        *t0 = 2 * c / (- b + root_discriminant);
        *t1 = (- b + root_discriminant) / (2 * a);
    }
    return true;
}

void sphere_intersect_func(const RTCIntersectFunctionNArguments* args) {
    assert(args->N == 1);
    int *valid = args->valid;
    if (!valid[0]) {
        return;
    }
    void *ptr = args->geometryUserPtr;
    const Sphere *sphere = (const Sphere*)ptr;
    RTCRayHitN *rayhit = args->rayhit;
    RTCRay *rtc_ray = (RTCRay*)RTCRayHitN_RayN(rayhit, 1);
    RTCHit *rtc_hit = (RTCHit*)RTCRayHitN_HitN(rayhit, 1);

    Ray ray{Vector3{rtc_ray->org_x, rtc_ray->org_y, rtc_ray->org_z},
            Vector3{rtc_ray->dir_x, rtc_ray->dir_y, rtc_ray->dir_z},
            rtc_ray->tnear, rtc_ray->tfar};
    // Our sphere is ||p - x||^2 = r^2
    // substitute x = o + d * t, we want to solve for t
    // ||p - (o + d * t)||^2 = r^2
    // (p.x - (o.x + d.x * t))^2 + (p.y - (o.y + d.y * t))^2 + (p.z - (o.z + d.z * t))^2 - r^2 = 0
    // (d.x^2 + d.y^2 + d.z^2) t^2 + 2 * (d.x * (o.x - p.x) + d.y * (o.y - p.y) + d.z * (o.z - p.z)) t + 
    // ((p.x-o.x)^2 + (p.y-o.y)^2 + (p.z-o.z)^2  - r^2) = 0
    // A t^2 + B t + C
    Vector3 v = ray.org - sphere->position;
    Real A = dot(ray.dir, ray.dir);
    Real B = 2 * dot(ray.dir, v);
    Real C = dot(v, v) - sphere->radius * sphere->radius;
    Real t0, t1;
    if (!solve_quadratic(A, B, C, &t0, &t1)) {
        // No intersection
        return;
    }
    assert(t0 <= t1);
    Real t = -1;
    if (t0 >= ray.tnear && t0 < ray.tfar) {
        t = t0;
    }
    if (t1 >= ray.tnear && t1 < ray.tfar && t < 0) {
        t = t1;
    }

    if (t >= ray.tnear && t < ray.tfar) {
        // Record the intersection
        Vector3 p = ray.org + t0 * ray.dir;
        Vector3 geometry_normal = p - sphere->position;
        rtc_hit->Ng_x = geometry_normal.x;
        rtc_hit->Ng_y = geometry_normal.y;
        rtc_hit->Ng_z = geometry_normal.z;
        // We use the spherical coordinates as uv
        Vector3 cartesian = geometry_normal / sphere->radius;
        // https://en.wikipedia.org/wiki/Spherical_coordinate_system#Cartesian_coordinates
        // We use the convention that y is up axis.
        Real elevation = acos(cartesian.y);
        Real azimuth = atan2(cartesian.z, cartesian.x);
        rtc_hit->u = azimuth / c_TWOPI;
        rtc_hit->v = elevation / c_PI;
        rtc_hit->primID = args->primID;
        rtc_hit->geomID = args->geomID;
        rtc_hit->instID[0] = args->context->instID[0];
        rtc_ray->tfar = t0;
    }
}

void sphere_occluded_func(const RTCOccludedFunctionNArguments* args) {
    assert(args->N == 1);
    int *valid = args->valid;
    if (!valid[0]) {
        return;
    }

    void *ptr = args->geometryUserPtr;
    const Sphere *sphere = (const Sphere*)ptr;
    RTCRay *rtc_ray = (RTCRay *)args->ray;
    
    Ray ray{Vector3{rtc_ray->org_x, rtc_ray->org_y, rtc_ray->org_z},
            Vector3{rtc_ray->dir_x, rtc_ray->dir_y, rtc_ray->dir_z},
            rtc_ray->tnear, rtc_ray->tfar};

    // See sphere_intersect_func for explanation.
    Vector3 v = ray.org - sphere->position;
    Real A = dot(ray.dir, ray.dir);
    Real B = 2 * dot(ray.dir, v);
    Real C = dot(v, v) - sphere->radius * sphere->radius;
    Real t0, t1;
    if (!solve_quadratic(A, B, C, &t0, &t1)) {
        // No intersection
        return;
    }

    assert(t0 <= t1);
    Real t = -1;
    if (t0 >= ray.tnear && t0 < ray.tfar) {
        t = t0;
    }
    if (t1 >= ray.tnear && t1 < ray.tfar && t < 0) {
        t = t1;
    }

    if (t >= ray.tnear && t < ray.tfar) {
        rtc_ray->tfar = -infinity<float>();
    }
}

///////////////////////////////////////////////////////////////////////////
struct register_embree_op {
    uint32_t operator()(const Sphere &sphere) const;
    uint32_t operator()(const TriangleMesh &mesh) const;

    const RTCDevice &device;
    const RTCScene &scene;
};
uint32_t register_embree_op::operator()(const Sphere &sphere) const {
    RTCGeometry rtc_geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_USER);
    uint32_t geomID = rtcAttachGeometry(scene, rtc_geom);
    rtcSetGeometryUserPrimitiveCount(rtc_geom, 1);
    rtcSetGeometryUserData(rtc_geom, (void *)&sphere);
    rtcSetGeometryBoundsFunction(rtc_geom, sphere_bounds_func, nullptr);
    rtcSetGeometryIntersectFunction(rtc_geom, sphere_intersect_func);
    rtcSetGeometryOccludedFunction(rtc_geom, sphere_occluded_func);
    rtcCommitGeometry(rtc_geom);
    rtcReleaseGeometry(rtc_geom);
    return geomID;
}
uint32_t register_embree_op::operator()(const TriangleMesh &mesh) const {
    RTCGeometry rtc_geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    uint32_t geomID = rtcAttachGeometry(scene, rtc_geom);
    Vector4f *positions = (Vector4f*)rtcSetNewGeometryBuffer(
        rtc_geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3,
        sizeof(Vector4f), mesh.positions.size());
    Vector3i *triangles = (Vector3i*)rtcSetNewGeometryBuffer(
        rtc_geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
        sizeof(Vector3i), mesh.indices.size());
    for (int i = 0; i < (int)mesh.positions.size(); i++) {
        Vector3 position = mesh.positions[i];
        positions[i] = Vector4f{(float)position[0], (float)position[1], (float)position[2], 0.f};
    }
    for (int i = 0; i < (int)mesh.indices.size(); i++) {
        triangles[i] = mesh.indices[i];
    }
    rtcSetGeometryVertexAttributeCount(rtc_geom, 1);
    rtcCommitGeometry(rtc_geom);
    rtcReleaseGeometry(rtc_geom);
    return geomID;
}
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
struct sample_point_on_shape_op {
    PointAndNormal operator()(const Sphere &sphere) const;
    PointAndNormal operator()(const TriangleMesh &mesh) const;

    const Vector2 &uv; // for selecting a point on a 2D surface
    const Real &w; // for selecting triangles
};
PointAndNormal sample_point_on_shape_op::operator()(const Sphere &sphere) const {
    // https://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations#UniformSampleSphere
    Real z = 1 - 2 * uv.x;
    Real r = sqrt(fmax(Real(0), 1 - z * z));
    Real phi = 2 * c_PI * uv.y;
    Vector3 offset(r * cos(phi), r * sin(phi), z);
    Vector3 position = sphere.position + sphere.radius * offset;
    Vector3 normal = offset;
    return PointAndNormal{position, normal};
}
PointAndNormal sample_point_on_shape_op::operator()(const TriangleMesh &mesh) const {
    int tri_id = sample(mesh.triangle_sampler, w);
    assert(tri_id >= 0 && tri_id < (int)mesh.indices.size());
    Vector3i index = mesh.indices[tri_id];
    Vector3 v0 = mesh.positions[index[0]];
    Vector3 v1 = mesh.positions[index[1]];
    Vector3 v2 = mesh.positions[index[2]];
    Vector3 e1 = v1 - v0;
    Vector3 e2 = v2 - v0;
    // https://pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations#SamplingaTriangle
    Real a = sqrt(std::clamp(uv[0], Real(0), Real(1)));
    Real b1 = 1 - a;
    Real b2 = a * uv[1];
    return PointAndNormal{v0 + (e1 * b1) + (e2 * b2), normalize(cross(e1, e2))};
}
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
struct surface_area_op {
    Real operator()(const Sphere &sphere) const;
    Real operator()(const TriangleMesh &mesh) const;
};
Real surface_area_op::operator()(const Sphere &sphere) const {
    return 4 * c_PI * sphere.radius * sphere.radius;
}
Real surface_area_op::operator()(const TriangleMesh &mesh) const {
    return mesh.total_area;
}
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
struct pdf_point_on_shape_op {
    Real operator()(const Sphere &sphere) const;
    Real operator()(const TriangleMesh &mesh) const;
};
Real pdf_point_on_shape_op::operator()(const Sphere &sphere) const {
    return 1 / surface_area_op{}(sphere);
}
Real pdf_point_on_shape_op::operator()(const TriangleMesh &mesh) const {
    return 1 / surface_area_op{}(mesh);
}
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
struct init_sampling_dist_op {
    void operator()(Sphere &sphere) const;
    void operator()(TriangleMesh &mesh) const;
};
void init_sampling_dist_op::operator()(Sphere &sphere) const {
}
void init_sampling_dist_op::operator()(TriangleMesh &mesh) const {
    std::vector<Real> tri_areas(mesh.indices.size(), Real(0));
    Real total_area = 0;
    for (int tri_id = 0; tri_id < (int)mesh.indices.size(); tri_id++) {
        Vector3i index = mesh.indices[tri_id];
        Vector3 v0 = mesh.positions[index[0]];
        Vector3 v1 = mesh.positions[index[1]];
        Vector3 v2 = mesh.positions[index[2]];
        Vector3 e1 = v1 - v0;
        Vector3 e2 = v2 - v0;
        tri_areas[tri_id] = length(cross(e1, e2)) / 2;
        total_area += tri_areas[tri_id];
    }
    mesh.triangle_sampler = make_table_dist_1d(tri_areas);
    mesh.total_area = total_area;
}
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
struct compute_shading_info_op {
    ShadingInfo operator()(const Sphere &sphere) const;
    ShadingInfo operator()(const TriangleMesh &mesh) const;

    const PathVertex &vertex;
};
ShadingInfo compute_shading_info_op::operator()(const Sphere &sphere) const {
    // To compute the shading frame, we use the geometry normal as normal,
    // and dpdu as one of the tangent vector. 
    // We use the azimuthal angle as u, and the elevation as v, 
    // thus the point p on sphere and u, v has the following relationship:
    // p = center + {r * cos(u) * sin(v), r * sin(u) * sin(v), r * cos(v)}
    // thus dpdu = {-r * sin(u) * sin(v), r * cos(u) * sin(v), 0}
    //      dpdv = { r * cos(u) * cos(v), r * sin(u) * cos(v), - r * sin(v)}
    Vector3 dpdu{-sphere.radius * sin(vertex.st[0]) * sin(vertex.st[1]),
                  sphere.radius * cos(vertex.st[0]) * sin(vertex.st[1]),
                 Real(0)};
    // normalize for shading frame calculation
    Vector3 tangent = normalize(dpdu);
    Frame shading_frame(tangent,
                        normalize(cross(vertex.geometry_normal, tangent)),
                        vertex.geometry_normal);
    return ShadingInfo{vertex.st,
                       shading_frame,
                       1 / sphere.radius /* mean curvature */};
}
ShadingInfo compute_shading_info_op::operator()(const TriangleMesh &mesh) const {
    // Get UVs of the three vertices
    assert(vertex.primitive_id >= 0);
    Vector3i index = mesh.indices[vertex.primitive_id];
    Vector2 uvs[3];
    if (mesh.uvs.size() > 0) {
        uvs[0] = mesh.uvs[index[0]];
        uvs[1] = mesh.uvs[index[1]];
        uvs[2] = mesh.uvs[index[2]];
    } else {
        // Use barycentric coordinates
        uvs[0] = Vector2{0, 0};
        uvs[1] = Vector2{1, 0};
        uvs[2] = Vector2{1, 1};
    }
    // Barycentric coordinates are stored in vertex.st
    Vector2 uv = (1 - vertex.st[0] - vertex.st[1]) * uvs[0] +
                 vertex.st[0] * uvs[1] +
                 vertex.st[1] * uvs[2];
    Vector3 p0 = mesh.positions[index[0]],
            p1 = mesh.positions[index[1]],
            p2 = mesh.positions[index[2]];
    // We want to derive dp/du & dp/dv. We have the following
    // relation:
    // p  = (1 - s - t) * p0   + s * p1   + t * p2
    // uv = (1 - s - t) * uvs0 + s * uvs1 + t * uvs2
    // dp/duv = dp/dst * dst/duv = dp/dst * (duv/dst)^{-1}
    // where dp/dst is a 3x2 matrix, duv/dst and dst/duv is a 2x2 matrix,
    // and dp/duv is a 3x2 matrix.

    // Let's build duv/dst first. To be clearer, it is
    // [du/ds, du/dt]
    // [dv/ds, dv/dt]
    Vector2 duvds = uvs[2] - uvs[0];
    Vector2 duvdt = uvs[2] - uvs[1];
    // The inverse of this matrix is
    // (1/det) [ dv/dt, -du/dt]
    //         [-dv/ds,  du/ds]
    // where det = duds * dvdt - dudt * dvds
    Real det = duvds[0] * duvdt[1] - duvdt[0] * duvds[1];
    Real dsdu =  duvdt[1] / det;
    Real dtdu = -duvds[1] / det;
    Real dsdv =  duvdt[0] / det;
    Real dtdv = -duvds[0] / det;
    Vector3 dpdu;
    if (fabs(det) > 1e-8f) {
        // Now we just need to do the matrix multiplication
        Vector3 dpds = p2 - p0;
        Vector3 dpdt = p2 - p1;
        dpdu = dpds * dsdu + dpdt * dtdu;
        // dpdv = dpds * dsdv + dpdt * dtdv;
    } else {
        // degenerate uvs. Use an arbitrary coordinate system
        dpdu = coordinate_system(vertex.geometry_normal).first;
    }

    // normalize for shading frame calculation
    Vector3 tangent = normalize(dpdu);

    // Now let's get the shading normal & mean_curvature.
    // By default it is the geometry normal and we have zero curvature.
    Vector3 shading_normal = vertex.geometry_normal;
    Real mean_curvature = 0;
    // However if we have vertex normals, that overrides the geometry normal.
    if (mesh.normals.size() > 0) {
        Vector3 n0 = mesh.normals[index[0]],
                n1 = mesh.normals[index[1]],
                n2 = mesh.normals[index[2]];
        shading_normal = normalize(
            (1 - vertex.st[0] - vertex.st[1]) * n0 + 
                                vertex.st[0] * n1 +
                                vertex.st[1] * n2);
        // We want to compute dn/du & dn/dv for mean curvature.
        // This is computed in a similar way to dpdu.
        // dn/duv = dn/dst * dst/duv = dn/dst * (duv/dst)^{-1}
        Vector3 dnds = n2 - n0;
        Vector3 dndt = n2 - n1;
        Vector3 dndu = dnds * dsdu + dndt * dtdu;
        Vector3 dndv = dnds * dsdv + dndt * dtdv;
        Vector3 bitangent = normalize(cross(shading_normal, tangent));
        mean_curvature = (dot(dndu, tangent) + 
                          dot(dndv, bitangent)) / Real(2);
    }

    Frame shading_frame(tangent,
                        cross(shading_normal, tangent),
                        shading_normal);
    return ShadingInfo{uv, shading_frame, mean_curvature};
}
///////////////////////////////////////////////////////////////////////////

uint32_t register_embree(const Shape &shape, const RTCDevice &device, const RTCScene &scene) {
    return std::visit(register_embree_op{device, scene}, shape);
}

PointAndNormal sample_point_on_shape(const Shape &shape, const Vector2 &uv, Real w) {
    return std::visit(sample_point_on_shape_op{uv, w}, shape);
}

Real pdf_point_on_shape(const Shape &shape) {
    return std::visit(pdf_point_on_shape_op{}, shape);
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
