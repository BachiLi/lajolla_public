#pragma once

#include "spectrum.h"
#include <variant>

struct Ray;
struct pcg32_state;

struct HomogeneousMedium {
    Spectrum sigma_a, sigma_s;
};

using Medium = std::variant<HomogeneousMedium>;

/// Given a ray, sample a distance along the ray direction that is smaller or equal then the 
/// maxt of the ray, based on the transmittance of the medium.
/// Some medium sampling algorithms require an indefinitely long stream of 
/// random numbers, so we pass in a PCG random number generator instead
/// of just a few random numbers. Stratification for transmittance sampling
/// is an unsolved research problem.
Real sample_distance(const Medium &m, const Ray &ray, pcg32_state &rng);
