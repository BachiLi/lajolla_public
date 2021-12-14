#pragma once

#include "lajolla.h"
#include "intersection.h"
#include <variant>

template <typename T>
struct ConstantTexture {
    T value;
};

template <typename T>
using Texture = std::variant<ConstantTexture<T>>;

template <typename T>
struct eval_texture_op {
    T operator()(const ConstantTexture<T> &t) const;

    const PathVertex &vertex;
};
template <typename T>
T eval_texture_op<T>::operator()(const ConstantTexture<T> &t) const {
    return t.value;
}

template <typename T>
T eval(const Texture<T> &texture, const PathVertex &vertex) {
    return std::visit(eval_texture_op<T>{vertex}, texture);
}

inline ConstantTexture<Spectrum> make_constant_spectrum_texture(const Spectrum &spec) {
    return ConstantTexture<Spectrum>{spec};
}
