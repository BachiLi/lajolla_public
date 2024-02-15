#include "scene.h"
#include "table_dist.h"

Scene::Scene(const RTCDevice &embree_device,
             const Camera &camera,
             const std::vector<Material> &materials,
             const std::vector<Shape> &shapes,
             const std::vector<Light> &lights,
             const std::vector<Medium> &media,
             int envmap_light_id,
             const TexturePool &texture_pool,
             const RenderOptions &options,
             const std::string &output_filename) : 
        embree_device(embree_device), camera(camera), materials(materials),
        shapes(shapes), lights(lights), media(media),
        envmap_light_id(envmap_light_id),
        texture_pool(texture_pool), options(options),
        output_filename(output_filename) {
    // Register the geometry to Embree
    embree_scene = rtcNewScene(embree_device);
    // We don't care about build time.
    rtcSetSceneBuildQuality(embree_scene, RTC_BUILD_QUALITY_HIGH);
    rtcSetSceneFlags(embree_scene, RTC_SCENE_FLAG_ROBUST);
    for (const Shape &shape : this->shapes) {
        register_embree(shape, embree_device, embree_scene);
    }
    rtcCommitScene(embree_scene);

    // Get scene bounding box from Embree
    RTCBounds embree_bounds;
    rtcGetSceneBounds(embree_scene, &embree_bounds);
    Vector3 lb{embree_bounds.lower_x, embree_bounds.lower_y, embree_bounds.lower_z};
    Vector3 ub{embree_bounds.upper_x, embree_bounds.upper_y, embree_bounds.upper_z};
    bounds = BSphere{distance(ub, lb) / 2, (lb + ub) / Real(2)};

    // build shape & light sampling distributions if necessary
    // TODO: const_cast is a bit ugly...
    std::vector<Shape> &mod_shapes = const_cast<std::vector<Shape>&>(this->shapes);
    for (Shape &shape : mod_shapes) {
        init_sampling_dist(shape);
    }
    std::vector<Light> &mod_lights = const_cast<std::vector<Light>&>(this->lights);
    for (Light &light : mod_lights) {
        init_sampling_dist(light, *this);
    }

    // build a sampling distributino for all the lights
    std::vector<Real> power(this->lights.size());
    for (int i = 0; i < (int)this->lights.size(); i++) {
        power[i] = light_power(this->lights[i], *this);
    }
    light_dist = make_table_dist_1d(power);
}

Scene::~Scene() {
    // This decreses the reference count of embree_scene in Embree,
    // if it reaches zero, Embree will deallocate the scene.
    rtcReleaseScene(embree_scene);
}

int sample_light(const Scene &scene, Real u) {
    return sample(scene.light_dist, u);
}

Real light_pmf(const Scene &scene, int light_id) {
    return pmf(scene.light_dist, light_id);
}
