#pragma once

#include "lajolla.h"

constexpr int c_max_mipmap_levels = 8;

template <typename T>
struct Mipmap {
    std::vector<Image<T>> images;
};

template <typename T>
inline int get_width(const Mipmap<T> &mipmap) {
    assert(mipmap.images.size() > 0);
    return mipmap.images[0].width;
}

template <typename T>
inline int get_height(const Mipmap<T> &mipmap) {
    assert(mipmap.images.size() > 0);
    return mipmap.images[0].height;
}

template <typename T>
inline Mipmap<T> make_mipmap(const Image<T> &img) {
    Mipmap<T> mipmap;
    int size = max(img.width, img.height);
    int num_levels = std::min((int)ceil(log2(Real(img.width)) + 1), c_max_mipmap_levels);
    mipmap.images.push_back(img);
    for (int i = 1; i < num_levels; i++) {
        const Image<T> &prev_img = mipmap.images.back();
        int next_w = max(prev_img.width / 2, 1);
        int next_h = max(prev_img.height / 2, 1);
        Image<T> next_img(next_w, next_h);
        for (int y = 0; y < next_img.height; y++) {
            for (int x = 0; x < next_img.width; x++) {
                // 2x2 box filter
                next_img(x, y) =
                    (prev_img(2 * x    , 2 * y    ) +
                     prev_img(2 * x + 1, 2 * y    ) +
                     prev_img(2 * x    , 2 * y + 1) +
                     prev_img(2 * x + 1, 2 * y + 1)) / Real(4);
            }
        }
        mipmap.images.push_back(next_img);
    }
    return mipmap;
}

template <typename T>
inline T lookup(const Mipmap<T> &mipmap, Real x, Real y, int level) {
    assert(level >= 0 && level < (int)mipmap.images.size());
    // Bilinear interpolation
    x *= mipmap.images[level].width;
    y *= mipmap.images[level].height;
    int xfi = modulo(int(x), mipmap.images[level].width);
    int yfi = modulo(int(y), mipmap.images[level].height);
    int xci = modulo(xfi + 1, mipmap.images[level].width);
    int yci = modulo(yfi + 1, mipmap.images[level].height);
    Real x_off = x - xfi;
    Real y_off = y - yfi;
    T vff = mipmap.images[level](xfi, yfi);
    T vfc = mipmap.images[level](xfi, yci);
    T vcf = mipmap.images[level](xci, yfi);
    T vcc = mipmap.images[level](xci, yci);
    return vff * (1 - x_off) * (1 - y_off) +
           vfc * (1 - x_off) * y_off +
           vcf *      x_off  * (1 - y_off) +
           vcc *      x_off  *      y_off;
}

template <typename T>
inline T lookup(const Mipmap<T> &mipmap, Real x, Real y, Real level) {
    if (level <= 0) {
        return lookup(mipmap, x, y, 0);
    } else if (level < Real(mipmap.images.size() - 1)) {
        int flevel = std::clamp((int)floor(level), 0, (int)mipmap.images.size() - 1);
        int clevel = std::clamp(flevel + 1, 0, (int)mipmap.images.size() - 1);
        Real level_off = level - flevel;
        return lookup(mipmap, x, y, flevel) * (1 - level_off) +
               lookup(mipmap, x, y, clevel) *      level_off;
    } else {
        return lookup(mipmap, x, y, int(mipmap.images.size() - 1));
    }
}

using Mipmap1 = Mipmap<Real>;
using Mipmap3 = Mipmap<Vector3>;
