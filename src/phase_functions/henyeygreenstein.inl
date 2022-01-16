#include "../frame.h"

Spectrum eval_op::operator()(const HenyeyGreenstein &p) const {
    return make_const_spectrum(c_INVFOURPI *
        (1 - p.g * p.g) / 
            (pow((1 + p.g * p.g + 2 * p.g * dot(dir_in, dir_out)), Real(3)/Real(2))));
}

std::optional<Vector3> sample_phase_function_op::operator()(const HenyeyGreenstein &p) const {
    // We want to importance sample
    // p = 1/4pi * ((1 - g^2) / (1 + g^2 + 2 g cos_elevation)^(3/2))
    // the spherical integral is
    //   1/4pi \int \int ((1 - g^2) / (1 + g^2 + 2 g cos_elevation)^(3/2)) sin_elevation d elevation d azimuth
    // = 1/2 \int ((1 - g^2) / (1 + g^2 + 2 g cos_elevation)^(3/2)) sin_elevation d elevation
    // (throw in mathematica https://www.wolframalpha.com/input/?i=integrate+%281+-+g%5E2%29+%2F+%281+%2B+g%5E2+%2B+2+*+g+*+cos%28x%29%29%5E%283%2F2%29+sin%28x%29+dx)
    // = (1/2)(-(g^2 - 1) / (g sqrt(g^2 + 2 * g * cos(elevation) + 1)) + (g^2 - 1) / (g sqrt(g^2 - 2 * g + 1)))
    // = (1/2)(-(g^2 - 1) / (g sqrt(g^2 + 2 * g * cos(elevation) + 1)) + (g + 1) / g) = u
    // Now let's try to write cos(elevation) in terms of u
    // (g^2 - 1) / (sqrt(g^2 + 2 * g * cos(elevation) + 1)) = 2ug - (g + 1)
    // (g^2 - 1) / (2ug - (g + 1)) = sqrt(g^2 + 2 * g * cos(elevation) + 1)
    // ((g^2 - 1) / (2ug - (g + 1)))^2 = g^2 + 2 * g * cos(elevation) + 1
    // cos(elevation) = (((g^2 - 1) / (2ug - (g + 1)))^2 - (1 + g^2)) / 2g

    // When g ~= 0, this equation degenerates, thus we switch to uniform sphere
    // sampling when that happens
    if (fabs(p.g) < Real(1e-3)) { // the constant was taken from pbrt
        Real z = 1 - 2 * rnd_param.x;
        Real r = sqrt(fmax(Real(0), 1 - z * z));
        Real phi = 2 * c_PI * rnd_param.y;
        return Vector3{r * cos(phi), r * sin(phi), z};
    } else {
        Real tmp = (p.g * p.g - 1) / (2 * rnd_param.x * p.g - (p.g + 1));
        Real cos_elevation = (tmp * tmp - (1 + p.g * p.g)) / (2 * p.g);
        Real sin_elevation = sqrt(max(1 - cos_elevation * cos_elevation, Real(0)));
        Real azimuth = 2 * c_PI * rnd_param.y;
        Frame frame(dir_in);
        return to_world(frame,
            Vector3{sin_elevation * cos(azimuth),
                    sin_elevation * sin(azimuth),
                    cos_elevation});
    }
}

Real pdf_sample_phase_op::operator()(const HenyeyGreenstein &p) const {
    return c_INVFOURPI *
        (1 - p.g * p.g) / 
            (pow((1 + p.g * p.g + 2 * p.g * dot(dir_in, dir_out)), Real(3)/Real(2)));
}
