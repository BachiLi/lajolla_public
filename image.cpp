#include "image.h"

#include <algorithm>
#include <fstream>

using std::string;
using std::vector;

// Stolen from https://github.com/halide/Halide/blob/c6529edb23b9fab8b406b28a4f9ea05b08f81cfe/src/Util.cpp#L253
inline bool ends_with(const string &str, const string &suffix) {
    if (str.size() < suffix.size()) {
        return false;
    }
    size_t off = str.size() - suffix.size();
    for (size_t i = 0; i < suffix.size(); i++) {
        if (str[off + i] != suffix[i]) {
            return false;
        }
    }
    return true;
}

void imwrite(const string &filename, const Image3 &image) {
    if (ends_with(filename, ".pfm")) {
        std::ofstream ofs(filename.c_str(), std::ios::binary);
        ofs << "PF" << std::endl;
        ofs << image.width << " " << image.height << std::endl;
        ofs << "-1" << std::endl;
        // Convert image to float
        vector<Vector3f> data;
        std::transform(image.data.cbegin(), image.data.cend(), std::back_inserter(data),
            [] (const Vector3 &v) {return Vector3f(v.x, v.y, v.z);});
        ofs.write((const char *)data.data(), data.size() * sizeof(Vector3f));
    }
}
