#include "parallel.h"
#include "parse_scene.h"
#include "image.h"
#include "render.h"
#include <embree3/rtcore.h>
#include <memory>
#include <thread>
#include <vector>

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        std::cout << "[Usage] ./lajolla [-t num_threads] filename.xml" << std::endl;
        return 0;
    }

    int num_threads = std::thread::hardware_concurrency();
    std::vector<std::string> filenames;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-t") {
            num_threads = std::stoi(std::string(argv[++i]));
        } else {
            filenames.push_back(std::string(argv[i]));
        }
    }

    RTCDevice embree_device = rtcNewDevice(nullptr);
    parallel_init(num_threads);

    for (const std::string &filename : filenames) {
        Scene scene = parse_scene(filename, embree_device);
        std::shared_ptr<Image3> img = render(scene);
        imwrite(scene.output_filename, *img);
    }

    parallel_cleanup();
    rtcReleaseDevice(embree_device);
    return 0;
}
