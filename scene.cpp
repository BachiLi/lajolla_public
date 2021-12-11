#include "scene.h"

Scene::Scene(const RTCDevice &embree_device,
             const Camera &camera,
             const std::vector<std::shared_ptr<const Shape>> &shapes) : 
        embree_device(embree_device), camera(camera), shapes(shapes) {
    embree_scene = rtcNewScene(embree_device);
    // We don't care about build time.
    rtcSetSceneBuildQuality(embree_scene, RTC_BUILD_QUALITY_HIGH);
    rtcSetSceneFlags(embree_scene, RTC_SCENE_FLAG_ROBUST);
    for (std::shared_ptr<const Shape> shape : shapes) {
        shape->register_embree(embree_device, embree_scene);
    }
    rtcCommitScene(embree_scene);
}

Scene::~Scene() {
    rtcReleaseScene(embree_scene);
}
