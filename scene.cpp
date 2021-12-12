#include "scene.h"

Scene::Scene(const RTCDevice &embree_device,
             const Camera &camera,
             const std::vector<Material> &materials,
             const std::vector<Shape> &shapes,
             const std::vector<Light> &lights) : 
        embree_device(embree_device), camera(camera), materials(materials), shapes(shapes), lights(lights) {
    // Register the geometry to Embree
    embree_scene = rtcNewScene(embree_device);
    // We don't care about build time.
    rtcSetSceneBuildQuality(embree_scene, RTC_BUILD_QUALITY_HIGH);
    rtcSetSceneFlags(embree_scene, RTC_SCENE_FLAG_ROBUST);
    for (const Shape &shape : shapes) {
        std::visit(register_embree{embree_device, embree_scene}, shape);
    }
    rtcCommitScene(embree_scene);

    // build a sampling distributino for all the lights
    light_pmf.resize(lights.size());
    light_cdf.resize(lights.size() + 1);
    light_cdf[0] = 0;
    for (int i = 0; i < (int)lights.size(); i++) {
        Real power = std::visit(light_power{*this}, lights[i]);
        light_pmf[i] = power;
        light_cdf[i + 1] = light_cdf[i] + power;
    }
    Real total_power = light_cdf.back();
    if (total_power > 0) {
        for (int i = 0; i < (int)lights.size(); i++) {
            light_pmf[i] /= total_power;
            light_cdf[i] /= total_power;
        }
    }
}

Scene::~Scene() {
    rtcReleaseScene(embree_scene);
}

int sample_light(const Scene &scene, Real u) {
    int num_lights = scene.lights.size();
    assert(num_lights > 0);
    const Real *ptr = std::upper_bound(scene.light_cdf.data(), scene.light_cdf.data() + num_lights + 1, u);
    int offset = std::clamp(int(ptr - scene.light_cdf.data() - 1), 0, num_lights - 1);
    return offset;
}

Real light_pmf(const Scene &scene, int light_id) {
    assert(light_id >= 0 && light_id < (int)scene.lights.size());
    return scene.light_pmf[light_id];
}
