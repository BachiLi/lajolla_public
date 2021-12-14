#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>

// We use double for most of our computation.
// Rendering is usually done in single precision Reals.
// However, lajolla is a educational renderer with does not
// put emphasis on the absolute performance. 
// We choose double so that we do not need to worry about
// numerical accuracy as much when we render.
// Switching to Realing point computation is easy --
// just set Real = Real.
using Real = double;

// Lots of PIs!
const Real c_PI = Real(3.14159265358979323846);
const Real c_INVPI = Real(1.0) / c_PI;
const Real c_TWOPI = Real(2.0) * c_PI;
const Real c_INVTWOPI = Real(1.0) / c_TWOPI;
const Real c_FOURPI = Real(4.0) * c_PI;
const Real c_INVFOURPI = Real(1.0) / c_FOURPI;
const Real c_PIOVERTWO = Real(0.5) * c_PI;
const Real c_PIOVERFOUR = Real(0.25) * c_PI;

const Real c_shadow_epsilon = 1e-4;
const Real c_isect_epsilon = 1e-4;

template <typename T>
inline T infinity() {
    return std::numeric_limits<T>::infinity();
}

namespace fs = std::filesystem;
