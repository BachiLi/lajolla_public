#pragma once

#include "lajolla.h"
#include "camera.h"
#include "light.h"
#include "material.h"
#include "medium.h"
#include "shape.h"
#include "volume.h"

#include <memory>
#include <vector>

enum class Integrator {
    Depth,
    ShadingNormal,
    MeanCurvature,
    RayDifferential, // visualize radius & spread
    MipmapLevel,
    Path,
    VolPath
};

struct RenderOptions {
    Integrator integrator = Integrator::Path;
    int samples_per_pixel = 4;
    int max_depth = -1;
    int rr_depth = 5;
    int vol_path_version = 0;
    int max_null_collisions = 1000;
};

/// Bounding sphere
struct BSphere {
    Real radius;
    Vector3 center;
};

/// A "Scene" contains the camera, materials, geometry (shapes), lights,
/// and also the rendering options such as number of samples per pixel or
/// the parameters of our renderer.
struct Scene {
    Scene() {}
    Scene(const RTCDevice &embree_device,
          const Camera &camera,
          const std::vector<Material> &materials,
          const std::vector<Shape> &shapes,
          const std::vector<Light> &lights,
          const std::vector<Medium> &media,
          int envmap_light_id, /* -1 if the scene has no envmap */
          const TexturePool &texture_pool,
          const RenderOptions &options,
          const std::string &output_filename);
    ~Scene();
    Scene(const Scene& t) = delete;
    Scene& operator=(const Scene& t) = delete;

    RTCDevice embree_device;
    RTCScene embree_scene;
    // We decide to maintain a copy of the scene here.
    // This allows us to manage the memory of the scene ourselves and decouple
    // from the scene parser, but it's obviously less efficient.
    Camera camera;
    // For now we use stl vectors to store scene content.
    // This wouldn't work if we want to extend this to run on GPUs.
    // If we want to port this to GPUs later, we need to maintain a thrust vector or something similar.
    const std::vector<Material> materials;
    const std::vector<Shape> shapes;
    const std::vector<Light> lights;
    const std::vector<Medium> media;
    int envmap_light_id;
    const TexturePool texture_pool;

    // Bounding sphere of the scene.
    BSphere bounds;
    
    RenderOptions options;
    std::string output_filename;

    // For sampling lights
    TableDist1D light_dist;
};

/// Sample a light source from the scene given a random number u \in [0, 1]
int sample_light(const Scene &scene, Real u);

/// The probability mass function of the sampling procedure above.
Real light_pmf(const Scene &scene, int light_id);

inline bool has_envmap(const Scene &scene) {
    return scene.envmap_light_id != -1;
}

inline const Light &get_envmap(const Scene &scene) {
    assert(scene.envmap_light_id != -1);
    return scene.lights[scene.envmap_light_id];
}

inline Real get_shadow_epsilon(const Scene &scene) {
    return min(scene.bounds.radius * Real(1e-5), Real(0.01));
}

inline Real get_intersection_epsilon(const Scene &scene) {
    return min(scene.bounds.radius * Real(1e-5), Real(0.01));
}
