#include "camera.h"
#include "render.h"
#include "material.h"
#include "scene.h"
#include "shape.h"
#include "transform.h"
#include <embree3/rtcore.h>
#include <memory>
#include <vector>

int main(int argc, char *argv[]) {
    RTCDevice embree_device = rtcNewDevice(nullptr);
    int w = 256, h = 256;
    Camera camera(translate(Vector3{0, 0, -3}),
                  Real(45),
                  w, h);
    std::vector<Material> materials;
    materials.push_back(Lambertian{Vector3{0.3, 0.6, 0.3}});
    std::vector<Shape> shapes;
    shapes.push_back(Sphere{0 /* material ID */, -1 /* light ID */,
                            Vector3{0, 0, 0} /* position */, Real(1) /* radius */});
    shapes.push_back(Sphere{0 /* material ID */, 0 /* light ID */,
                            Vector3{0, 0, -5} /* position */, Real(0.2) /* radius */});
    std::vector<Light> lights;
    lights.push_back(DiffuseAreaLight{1 /* shape ID */, Vector3{80, 80, 80}});
    Scene scene{embree_device, camera, materials, shapes, lights};
    std::shared_ptr<Image3> img = render(scene);
    imwrite("out.pfm", *img);
    rtcReleaseDevice(embree_device);
    return 0;
}
