#include "phase_function.h"

struct eval_op {
    Spectrum operator()(const IsotropicPhase &p) const;
    Spectrum operator()(const HenyeyGreenstein &p) const;

    const Vector3 &dir_in;
    const Vector3 &dir_out;
};

struct sample_phase_function_op {
    std::optional<Vector3> operator()(const IsotropicPhase &p) const;
    std::optional<Vector3> operator()(const HenyeyGreenstein &p) const;

    const Vector3 &dir_in;
    const Vector2 &rnd_param;
};

struct pdf_sample_phase_op {
    Real operator()(const IsotropicPhase &p) const;
    Real operator()(const HenyeyGreenstein &p) const;

    const Vector3 &dir_in;
    const Vector3 &dir_out;
};

#include "phase_functions/isotropic.inl"
#include "phase_functions/henyeygreenstein.inl"

Spectrum eval(const PhaseFunction &phase_function,
              const Vector3 &dir_in,
              const Vector3 &dir_out) {
    return std::visit(eval_op{dir_in, dir_out}, phase_function);
}

std::optional<Vector3> sample_phase_function(
        const PhaseFunction &phase_function,
        const Vector3 &dir_in,
        const Vector2 &rnd_param) {
    return std::visit(sample_phase_function_op{dir_in, rnd_param}, phase_function);
}

Real pdf_sample_phase(const PhaseFunction &phase_function,
                      const Vector3 &dir_in,
                      const Vector3 &dir_out) {
    return std::visit(pdf_sample_phase_op{dir_in, dir_out}, phase_function);
}
