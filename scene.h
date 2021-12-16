#pragma once

#include "lajolla.h"
#include "camera.h"
#include "light.h"
#include "material.h"
#include "shape.h"

#include <memory>
#include <vector>

enum class Integrator {
    Depth,
    MeanCurvature,
    RayDifferential, // visualize radius & spread
    Path
};

struct RenderOptions {
    Integrator integrator = Integrator::Path;
    int samples_per_pixel = 4;
    int max_depth = 6;
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
          const TexturePool &texture_pool,
          const RenderOptions &options,
          const std::string &output_filename);
    ~Scene();

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
    const TexturePool texture_pool;
    
    RenderOptions options;
    std::string output_filename;

    // For sampling lights
    TableDist1D light_dist;
};

/// Sample a light source from the scene given a random number u \in [0, 1]
int sample_light(const Scene &scene, Real u);

/// The probability mass function of the sampling procedure above.
Real light_pmf(const Scene &scene, int light_id);
