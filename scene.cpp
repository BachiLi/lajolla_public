#include "scene.h"
#include "table_dist.h"

Scene::Scene(const RTCDevice &embree_device,
             const Camera &camera,
             const std::vector<Material> &materials,
             const std::vector<Shape> &shapes,
             const std::vector<Light> &lights,
             const RenderOptions &options,
             const std::string &output_filename) : 
        embree_device(embree_device), camera(camera), materials(materials),
        shapes(shapes), lights(lights), options(options),
        output_filename(output_filename) {
    // Register the geometry to Embree
    embree_scene = rtcNewScene(embree_device);
    // We don't care about build time.
    rtcSetSceneBuildQuality(embree_scene, RTC_BUILD_QUALITY_HIGH);
    rtcSetSceneFlags(embree_scene, RTC_SCENE_FLAG_ROBUST);
    for (const Shape &shape : shapes) {
        register_embree(shape, embree_device, embree_scene);
    }
    rtcCommitScene(embree_scene);

    // build a sampling distributino for all the lights
    std::vector<Real> power(lights.size());
    for (int i = 0; i < (int)lights.size(); i++) {
        power[i] = light_power(lights[i], *this);
    }
    light_dist = make_table_dist_1d(power);

    // build shape sampling distributions if necessary
    // TODO: const_cast is a bit ugly...
    std::vector<Shape> &mod_shapes = const_cast<std::vector<Shape>&>(this->shapes);
    for (Shape &shape : mod_shapes) {
        init_sampling_dist(shape);
    }
}

Scene::~Scene() {
    rtcReleaseScene(embree_scene);
}

int sample_light(const Scene &scene, Real u) {
    return sample(scene.light_dist, u);
}

Real light_pmf(const Scene &scene, int light_id) {
    return pmf(scene.light_dist, light_id);
}
