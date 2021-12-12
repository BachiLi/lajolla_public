#pragma once

#include "lajolla.h"
#include "camera.h"
#include "light.h"
#include "material.h"
#include "shape.h"

#include <memory>
#include <vector>

struct Scene {
    Scene(const RTCDevice &embree_device,
          const Camera &camera,
          const std::vector<Material> &materials,
          const std::vector<Shape> &shapes,
          const std::vector<Light> &lights);
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

    // For sampling lights
    std::vector<Real> light_pmf;
    std::vector<Real> light_cdf;
};

int sample_light(const Scene &scene, Real u);
Real light_pmf(const Scene &scene, int light_id);
