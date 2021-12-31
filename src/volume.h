#pragma once

#include "lajolla.h"
#include "ray.h"
#include "spectrum.h"
#include "vector.h"
#include <variant>
#include <vector>

/// For representing participating media data.
/// Will be used in homework 2. Can think of this as a 3D texture.

template <typename T>
struct ConstantVolume {
    T value;
};

template <typename T>
struct GridVolume {
    Vector3i resolution;
    // the bounding box of the grid
    Vector3 p_min, p_max;
    std::vector<T> data;
    T max_data;
    Real scale = 1;
};

template <typename T>
using Volume = std::variant<ConstantVolume<T>, GridVolume<T>>;
using Volume1 = Volume<Real>;
using VolumeSpectrum = Volume<Spectrum>;

template <typename T>
struct eval_volume_op {
    T operator()(const ConstantVolume<T> &v) const;
    T operator()(const GridVolume<T> &v) const;

    const Vector3 &p;
};
template <typename T>
T eval_volume_op<T>::operator()(const ConstantVolume<T> &v) const {
    return v.value;
}
template <typename T>
T eval_volume_op<T>::operator()(const GridVolume<T> &v) const {
    // Trilinear interpolation
    Vector3 pn = (p - v.p_min) / (v.p_max - v.p_min);
    if (pn.x < 0 || pn.x > 1 || pn.y < 0 || pn.y > 1 || pn.z < 0 || pn.z > 1) {
        return make_zero_spectrum();
    }
    pn.x *= Real(v.resolution.x - 1);
    pn.y *= Real(v.resolution.y - 1);
    pn.z *= Real(v.resolution.z - 1);
    int x0 = std::clamp(int(pn.x), 0, v.resolution.x-1);
    int y0 = std::clamp(int(pn.y), 0, v.resolution.y-1);
    int z0 = std::clamp(int(pn.z), 0, v.resolution.z-1);
    int x1 = std::clamp(x0 + 1, 0, v.resolution.x - 1);
    int y1 = std::clamp(y0 + 1, 0, v.resolution.y - 1);
    int z1 = std::clamp(z0 + 1, 0, v.resolution.z - 1);
    Real dx = pn.x - x0;
    Real dy = pn.y - y0;
    Real dz = pn.z - z0;
    assert(dx >= 0 && dx <= 1 && dy >= 0 && dy <= 1 && dz >= 0 && dz <= 1);
    T v000 = v.data[(z0 * v.resolution.y + y0) * v.resolution.x + x0];
    T v001 = v.data[(z0 * v.resolution.y + y0) * v.resolution.x + x1];
    T v010 = v.data[(z0 * v.resolution.y + y1) * v.resolution.x + x0];
    T v011 = v.data[(z0 * v.resolution.y + y1) * v.resolution.x + x1];
    T v100 = v.data[(z1 * v.resolution.y + y0) * v.resolution.x + x0];
    T v101 = v.data[(z1 * v.resolution.y + y0) * v.resolution.x + x1];
    T v110 = v.data[(z1 * v.resolution.y + y1) * v.resolution.x + x0];
    T v111 = v.data[(z1 * v.resolution.y + y1) * v.resolution.x + x1];
    return v.scale * (
           v000 * ((1 - dx) * (1 - dy) * (1 - dz)) +
           v001 * (     dx  * (1 - dy) * (1 - dz)) +
           v010 * ((1 - dx) *      dy  * (1 - dz)) +
           v011 * (     dx  *      dy  * (1 - dz)) +
           v100 * ((1 - dx) * (1 - dy) *      dz)  +
           v101 * (     dx  * (1 - dy) *      dz)  +
           v110 * ((1 - dx) *      dy  *      dz)  +
           v111 * (     dx  *      dy  *      dz));
}

template <typename T>
struct max_value_op {
    T operator()(const ConstantVolume<T> &v) const;
    T operator()(const GridVolume<T> &v) const;
};
template <typename T>
T max_value_op<T>::operator()(const ConstantVolume<T> &v) const {
    return v.value;
}
template <typename T>
T max_value_op<T>::operator()(const GridVolume<T> &v) const {
    return v.scale * v.max_data;
}

template <typename T>
struct set_scale_op {
    void operator()(ConstantVolume<T> &v) const;
    void operator()(GridVolume<T> &v) const;

    Real scale;
};
template <typename T>
void set_scale_op<T>::operator()(ConstantVolume<T> &v) const {
    v.value *= scale;
}
template <typename T>
void set_scale_op<T>::operator()(GridVolume<T> &v) const {
    v.scale = scale;
}

template <typename T>
struct intersect_op {
    bool operator()(const ConstantVolume<T> &v) const;
    bool operator()(const GridVolume<T> &v) const;

    const Ray &ray;
};
template <typename T>
bool intersect_op<T>::operator()(const ConstantVolume<T> &v) const {
    return true;
}
template <typename T>
bool intersect_op<T>::operator()(const GridVolume<T> &v) const {
    // https://github.com/mmp/pbrt-v3/blob/master/src/core/geometry.h#L1388
    Real t0 = 0, t1 = ray.tfar;
    for (int i = 0; i < 3; i++) {
        Real tnear = (v.p_min[i] - ray.org[i]) / ray.dir[i];
        Real tfar = (v.p_max[i] - ray.org[i]) / ray.dir[i];

        // Update parametric interval from slab intersection $t$ values
        if (tnear > tfar) {
            std::swap(tnear, tfar);
        }

        t0 = tnear > t0 ? tnear : t0;
        t1 = tfar < t1 ? tfar : t1;
        if (t0 > t1) {
            return false;
        }
    }
    return true;
}

template <typename T>
T lookup(const Volume<T> &volume, const Vector3 &p) {
    return std::visit(eval_volume_op<T>{p}, volume);
}

template <typename T>
T get_max_value(const Volume<T> &volume) {
    return std::visit(max_value_op<T>{}, volume);
}

template <typename T>
void set_scale(Volume<T> &v, Real scale) {
    std::visit(set_scale_op<T>{scale}, v);
}

template <typename T>
bool intersect(const Volume<T> &v, const Ray &ray) {
    return std::visit(intersect_op<T>{ray}, v);
}

template <typename T>
    GridVolume<T> load_volume_from_file(const fs::path &filename) {
    return GridVolume<T>{};
}

template <>
GridVolume<Real> load_volume_from_file(const fs::path &filename);

template <>
GridVolume<Spectrum> load_volume_from_file(const fs::path &filename);
