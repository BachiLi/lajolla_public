#pragma once

#include "phase_function.h"
#include "spectrum.h"
#include "volume.h"
#include <variant>

struct Scene;

struct MediumBase {
    PhaseFunction phase_function;
};

struct HomogeneousMedium : public MediumBase {
    Spectrum sigma_a, sigma_s;
};

struct HeterogeneousMedium : public MediumBase {
    VolumeSpectrum albedo, density;
};

using Medium = std::variant<HomogeneousMedium, HeterogeneousMedium>;

/// the maximum of sigma_t = sigma_s + sigma_a over the whole space
Spectrum get_majorant(const Medium &medium, const Ray &ray);
Spectrum get_sigma_s(const Medium &medium, const Vector3 &p);
Spectrum get_sigma_a(const Medium &medium, const Vector3 &p);

inline PhaseFunction get_phase_function(const Medium &medium) {
    return std::visit([&](const auto &m) { return m.phase_function; }, medium);
}
