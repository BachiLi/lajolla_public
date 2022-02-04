#include "parallel.h"
#include "parse_scene.h"
#include "image.h"
#include "render.h"
#include "timer.h"
#include <embree3/rtcore.h>
#include <memory>
#include <thread>
#include <vector>

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        std::cout << "[Usage] ./lajolla [-t num_threads] [-o output_file_name] filename.xml" << std::endl;
        return 0;
    }

    int num_threads = std::thread::hardware_concurrency();
    std::string outputfile = "";
    std::vector<std::string> filenames;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-t") {
            num_threads = std::stoi(std::string(argv[++i]));
        } if (std::string(argv[i]) == "-o") {
            outputfile = std::string(argv[++i]);
        } else {
            filenames.push_back(std::string(argv[i]));
        }
    }

    RTCDevice embree_device = rtcNewDevice(nullptr);
    parallel_init(num_threads);

    for (const std::string &filename : filenames) {
        Timer timer;
        tick(timer);
        std::cout << "Parsing and constructing scene " << filename << "." << std::endl;
        Scene scene = parse_scene(filename, embree_device);
        std::cout << "Done. Took " << tick(timer) << " seconds." << std::endl;
        std::cout << "Rendering..." << std::endl;
        Image3 img = render(scene);
        if (outputfile.compare("") == 0) {outputfile = scene.output_filename;}
        std::cout << "Done. Took " << tick(timer) << " seconds." << std::endl;
        imwrite(outputfile, img);
        std::cout << "Image written to " << outputfile << std::endl;
    }

    parallel_cleanup();
    rtcReleaseDevice(embree_device);
    return 0;
}

