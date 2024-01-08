#include "intersection.h"
#include "material.h"
#include "ray.h"
#include "scene.h"
#include <embree4/rtcore.h>

std::optional<PathVertex> intersect(const Scene &scene,
                                    const Ray &ray,
                                    const RayDifferential &ray_diff) {
    RTCIntersectArguments rtc_args;
    rtcInitIntersectArguments(&rtc_args);
    RTCRayHit rtc_rayhit;
    RTCRay &rtc_ray = rtc_rayhit.ray;
    RTCHit &rtc_hit = rtc_rayhit.hit;
    rtc_ray = RTCRay{
        (float)ray.org.x, (float)ray.org.y, (float)ray.org.z,
        (float)ray.tnear,
        (float)ray.dir.x, (float)ray.dir.y, (float)ray.dir.z,
        0.f, // time
        (float)ray.tfar,
        (unsigned int)(-1), // mask
        0, // ray ID
        0  // ray flags
    };
    rtc_hit = RTCHit{
        0, 0, 0, // Ng_x, Ng_y, Ng_z
        0, 0, // u, v
        RTC_INVALID_GEOMETRY_ID, // primitive ID
        RTC_INVALID_GEOMETRY_ID, // geometry ID
        {RTC_INVALID_GEOMETRY_ID} // instance IDs
    };
    rtcIntersect1(scene.embree_scene, &rtc_rayhit, &rtc_args);
    if (rtc_hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return {};
    };
    assert(rtc_hit.geomID < scene.shapes.size());

    PathVertex vertex;
    vertex.position = Vector3{ray.org.x, ray.org.y, ray.org.z} +
        Vector3{ray.dir.x, ray.dir.y, ray.dir.z} * Real(rtc_ray.tfar);
    vertex.geometric_normal = normalize(Vector3{rtc_hit.Ng_x, rtc_hit.Ng_y, rtc_hit.Ng_z});
    vertex.shape_id = rtc_hit.geomID;
    vertex.primitive_id = rtc_hit.primID;
    const Shape &shape = scene.shapes[vertex.shape_id];
    vertex.material_id = get_material_id(shape);
    vertex.interior_medium_id = get_interior_medium_id(shape);
    vertex.exterior_medium_id = get_exterior_medium_id(shape);
    vertex.st = Vector2{rtc_hit.u, rtc_hit.v};

    ShadingInfo shading_info = compute_shading_info(scene.shapes[vertex.shape_id], vertex);
    vertex.shading_frame = shading_info.shading_frame;
    vertex.uv = shading_info.uv;
    vertex.mean_curvature = shading_info.mean_curvature;
    vertex.ray_radius = transfer(ray_diff, distance(ray.org, vertex.position));
    // vertex.ray_radius stores approximatedly dp/dx, 
    // we get uv_screen_size (du/dx) using (dp/dx)/(dp/du)
    vertex.uv_screen_size = vertex.ray_radius / shading_info.inv_uv_size;

    // Flip the geometry normal to the same direction as the shading normal
    if (dot(vertex.geometric_normal, vertex.shading_frame.n) < 0) {
        vertex.geometric_normal = -vertex.geometric_normal;
    }

    return vertex;
}

bool occluded(const Scene &scene, const Ray &ray) {
    RTCOccludedArguments rtc_args;
    rtcInitOccludedArguments(&rtc_args);
    RTCRay rtc_ray;
    rtc_ray.org_x = (float)ray.org[0];
    rtc_ray.org_y = (float)ray.org[1];
    rtc_ray.org_z = (float)ray.org[2];
    rtc_ray.dir_x = (float)ray.dir[0];
    rtc_ray.dir_y = (float)ray.dir[1];
    rtc_ray.dir_z = (float)ray.dir[2];
    rtc_ray.tnear = (float)ray.tnear;
    rtc_ray.tfar = (float)ray.tfar;
    rtc_ray.mask = (unsigned int)(-1);
    rtc_ray.time = 0.f;
    rtc_ray.flags = 0;
    // TODO: switch to rtcOccluded16
    rtcOccluded1(scene.embree_scene, &rtc_ray, &rtc_args);
    return rtc_ray.tfar < 0;
}

Spectrum emission(const PathVertex &v,
                  const Vector3 &view_dir,
                  const Scene &scene) {
    int light_id = get_area_light_id(scene.shapes[v.shape_id]);
    assert(light_id >= 0);
    const Light &light = scene.lights[light_id];
    return emission(light,
                    view_dir,
                    v.uv_screen_size,
                    PointAndNormal{v.position, v.geometric_normal},
                    scene);
}
