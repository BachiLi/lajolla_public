#pragma once

#include "lajolla.h"
#include "vector.h"
#include <variant>

// Many common open source renderers implement pixel filtering using
// a "splatting" approach: they sample a point from a pixel, and then
// splat the contribution to all nearby pixels overlapped with the
// filter support.
// This approach works fine, but has a few disadvantages:
// 1) This introduces race conditions between different pixels, and requires atomic operations.
// 2) This introduces correlation between pixels and hurts denoising.
// 3) The splatting approach is biased and creates artifacts at low sampling rates.
// 4) For filters with infinite supports (e.g., Gaussian), 
//    this requires a discontinuous cutoff radius (otherwise it would be too slow).
// For these reasons, many modern production renderers have started to employ
// a different and simpler strategy.
// For each pixel, we solve for the pixel filter integral by directly importance
// sample that filter, and we *do not* share samples among pixels.
// This allows us to avoid all three problems above.
// This approach was described by Shirley et al. in 1991
// "A ray tracing framework for global illumination systems"
// and was discussed more recently by Ernst et al. in "Filter Importance Sampling".

// To make things simple, we only support filters with closed-form importance
// sampling distribution. This means that our sampling weight is always 1, since
// a pixel filter always normalize to 1.

struct Box {
    Real width;
};

struct Tent {
    Real width;
};

struct Gaussian {
    Real stddev;
};

// Exercise: implement "Generation of Stratified Samples for B-Spline Pixel Filtering"
// from Stark et al. for a B-spline filter.

using Filter = std::variant<Box, Tent, Gaussian>;

Vector2 sample(const Filter &filter, const Vector2 &rnd_param);
