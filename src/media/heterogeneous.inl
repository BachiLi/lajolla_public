#include "../volume.h"

Spectrum get_majorant_op::operator()(const HeterogeneousMedium &m) {
    if (intersect(m.density, ray)) {
        return get_max_value(m.density);
    } else {
        return make_zero_spectrum();
    }
}

Spectrum get_sigma_s_op::operator()(const HeterogeneousMedium &m) {
    Spectrum density = lookup(m.density, p);
    Spectrum albedo = lookup(m.albedo, p);
    return density * albedo;
}

Spectrum get_sigma_a_op::operator()(const HeterogeneousMedium &m) {
    Spectrum density = lookup(m.density, p);
    Spectrum albedo = lookup(m.albedo, p);
    return density * (Real(1) - albedo);
}
