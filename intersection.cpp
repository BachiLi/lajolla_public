#include "intersection.h"
#include "material.h"
#include "ray.h"
#include "scene.h"
#include <embree3/rtcore.h>

bool intersect(const Scene &scene, const Ray &ray, Intersection *isect) {
    RTCIntersectContext rtc_context;
    rtcInitIntersectContext(&rtc_context);
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
    rtcIntersect1(scene.embree_scene, &rtc_context, &rtc_rayhit);
    if (rtc_hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return false;
    };

    isect->position = Vector3{ray.org.x, ray.org.y, ray.org.z} +
        Vector3{ray.dir.x, ray.dir.y, ray.dir.z} * Real(rtc_ray.tfar);
    isect->geometry_normal = normalize(Vector3{rtc_hit.Ng_x, rtc_hit.Ng_y, rtc_hit.Ng_z});
    isect->material = &scene.materials[rtc_hit.geomID];
    return true;
}
