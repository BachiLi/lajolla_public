#pragma once

#include "vector.h"

/// For now, lajolla assumes we are operating in the linear and trimulus RGB color space.
/// In the future we might implement a proper spectral renderer.
using Spectrum = Vector3;

inline Spectrum make_zero_spectrum() {
    return Vector3{0, 0, 0};
}

inline Spectrum fromRGB(const Vector3 &rgb) {
    return rgb;
}

inline Real luminance(const Spectrum &s) {
    return s.x * Real(0.212671) + s.y * Real(0.715160) + s.z * Real(0.072169);
}

inline Vector3 toRGB(const Spectrum &s) {
    return s;
}
