#pragma once

#include "vector.h"

#include <string>
#include <vector>

/// A 3 channel image stored in a contiguous vector
struct Image3 {
    Image3() {}
    Image3(int w, int h) : width(w), height(h) {
        data.resize(w * h);
        fill(data.begin(), data.end(), Vector3(0, 0, 0));
    }

    Vector3 &operator()(int x) {
        return data[x];
    }

    const Vector3 &operator()(int x) const {
        return data[x];
    }

    Vector3 &operator()(int x, int y) {
        return data[y * width + x];
    }

    const Vector3 &operator()(int x, int y) const {
        return data[y * width + x];
    }

    int width;
    int height;
    std::vector<Vector3> data;
};

/// Save an image to a file. Supported formats: pfm
void imwrite(const std::string &filename, const Image3 &image);