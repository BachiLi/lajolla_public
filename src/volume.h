#pragma once

#include "lajolla.h"
#include "vector.h"
#include <vector>

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
    pn.x = std::clamp(pn.x * Real(v.resolution.x), Real(0), Real(v.resolution.x));
    pn.y = std::clamp(pn.y * Real(v.resolution.y), Real(0), Real(v.resolution.y));
    pn.z = std::clamp(pn.z * Real(v.resolution.z), Real(0), Real(v.resolution.z));
    int x0 = std::clamp(int(pn.x * v.resolution.x), 0, v.resolution.x-1);
    int y0 = std::clamp(int(pn.y * v.resolution.y), 0, v.resolution.y-1);
    int z0 = std::clamp(int(pn.z * v.resolution.z), 0, v.resolution.z-1);
    int x1 = std::clamp(x0 + 1, 0, v.resolution.x - 1);
    int y1 = std::clamp(y0 + 1, 0, v.resolution.y - 1);
    int z1 = std::clamp(z0 + 1, 0, v.resolution.z - 1);
    Real dx = pn.x - x0;
    Real dy = pn.y - y0;
    Real dz = pn.z - z0;
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
T lookup(const Volume<T> &volume, const Vector3 &p) {
    return std::visit(eval_volume_op<T>{p}, volume);
}

template <typename T>
T get_max_value(const Volume<T> &volume) {
    return std::visit(max_value_op<T>{}, volume);
}

template <typename T>
    GridVolume<T> load_volume_from_file(const fs::path &filename) {
    return GridVolume<T>{};
}

template <>
GridVolume<Real> load_volume_from_file(const fs::path &filename);

template <>
GridVolume<Spectrum> load_volume_from_file(const fs::path &filename);
