#include "camera.h"
#include "render.h"
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
    std::vector<Shape> shapes;
    shapes.push_back(Sphere{Vector3{0, 0, 0} /* position */, Real(1) /* radius */});
    Scene scene{embree_device, camera, shapes};
    std::shared_ptr<Image3> img = render(scene);
    imwrite("out.pfm", *img);
    rtcReleaseDevice(embree_device);
    return 0;
}
