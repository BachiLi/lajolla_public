#include "image.h"
#include "flexception.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define TINYEXR_USE_MINIZ 1
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"
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

Image1 imread1(const fs::path &filename) {
    Image1 img;
    std::string extension = to_lowercase(filename.extension().string());
    // JPG, PNG, TGA, BMP, PSD, GIF, HDR, PIC
    if (extension == ".jpg" ||
          extension == ".png" ||
          extension == ".tga" ||
          extension == ".bmp" ||
          extension == ".psd" ||
          extension == ".gif" ||
          extension == ".hdr" ||
          extension == ".pic") {
        int w, h, n;
        float *data = stbi_loadf(filename.c_str(), &w, &h, &n, 1);
        img = Image1(w, h);
        if (data == nullptr) {
            Error(std::string("Failure when loading image: ") + filename.string());
        }
        for (int i = 0; i < w * h; i++) {
            img(i) = data[i];
        }
        stbi_image_free(data);
    } else if (extension == ".exr") {
        float* data = nullptr;
        int width;
        int height;
        const char* err = nullptr;
        int ret = LoadEXR(&data, &width, &height, filename.c_str(), &err);
        if (ret != TINYEXR_SUCCESS) {
            std::cerr << "OpenEXR error: " << err << std::endl;
            FreeEXRErrorMessage(err);
            Error(std::string("Failure when loading image: ") + filename.string());
        }
        for (int i = 0; i < width * height; i++) {
            img(i) = (data[4 * i] + data[4 * i + 1] + data[4 * i + 2]) / 3;
        }
        free(data);
    } else {
        Error(std::string("Unsupported image format: ") + filename.string());
    }
    return img;
}

Image3 imread3(const fs::path &filename) {
    Image3 img;
    std::string extension = to_lowercase(filename.extension().string());
    // JPG, PNG, TGA, BMP, PSD, GIF, HDR, PIC
    if (extension == ".jpg" ||
          extension == ".png" ||
          extension == ".tga" ||
          extension == ".bmp" ||
          extension == ".psd" ||
          extension == ".gif" ||
          extension == ".hdr" ||
          extension == ".pic") {
        int w, h, n;
        float *data = stbi_loadf(filename.c_str(), &w, &h, &n, 3);
        img = Image3(w, h);
        if (data == nullptr) {
            Error(std::string("Failure when loading image: ") + filename.string());
        }
        int j = 0;
        for (int i = 0; i < w * h; i++) {
            img(i)[0] = data[j++];
            img(i)[1] = data[j++];
            img(i)[2] = data[j++];
        }
        stbi_image_free(data);
    } else if (extension == ".exr") {
        float* data = nullptr;
        int width;
        int height;
        const char* err = nullptr;
        int ret = LoadEXR(&data, &width, &height, filename.c_str(), &err);
        if (ret != TINYEXR_SUCCESS) {
            std::cerr << "OpenEXR error: " << err << std::endl;
            FreeEXRErrorMessage(err);
            Error(std::string("Failure when loading image: ") + filename.string());
        }
        img = Image3(width, height);
        for (int i = 0; i < width * height; i++) {
            img(i) = Vector3{data[4 * i], data[4 * i + 1], data[4 * i + 2]};
        }
        free(data);
    } else {
        Error(std::string("Unsupported image format: ") + filename.string());
    }
    return img;
}

void imwrite(const fs::path &filename, const Image3 &image) {
    if (ends_with(filename, ".pfm")) {
        std::ofstream ofs(filename.c_str(), std::ios::binary);
        ofs << "PF" << std::endl;
        ofs << image.width << " " << image.height << std::endl;
        ofs << "-1" << std::endl;
        // Convert image to float
        vector<Vector3f> data(image.data.size());
        std::transform(image.data.cbegin(), image.data.cend(), data.begin(),
            [] (const Vector3 &v) {return Vector3f(v.x, v.y, v.z);});
        ofs.write((const char *)data.data(), data.size() * sizeof(Vector3f));
    }
}
