#pragma once

#include "lajolla.h"
#include "spectrum.h"
#include "vector.h"
#include <optional>
#include <variant>

struct IsotropicPhase {
};

using PhaseFunction = std::variant<IsotropicPhase>;

Spectrum eval(const PhaseFunction &phase_function,
              const Vector3 &dir_in,
              const Vector3 &dir_out);

std::optional<Vector3> sample_phase_function(
    const PhaseFunction &phase_function,
    const Vector3 &dir_in,
    const Vector2 &rnd_param);

Real pdf_sample_phase(const PhaseFunction &phase_function,
                      const Vector3 &dir_in,
                      const Vector3 &dir_out);
