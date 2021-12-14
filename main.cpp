#include "parse_scene.h"
#include "image.h"
#include "render.h"
#include <embree3/rtcore.h>
#include <memory>
#include <vector>

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        return 0;
    }

    RTCDevice embree_device = rtcNewDevice(nullptr);

    Scene scene = parse_scene(argv[1], embree_device);
    std::shared_ptr<Image3> img = render(scene);
    imwrite(scene.output_filename, *img);

    rtcReleaseDevice(embree_device);
    return 0;
}
