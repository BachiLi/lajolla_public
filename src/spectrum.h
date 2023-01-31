#pragma once

#include "vector.h"
#include <vector>

/// For now, lajolla assumes we are operating in the linear and trimulus RGB color space.
/// In the future we might implement a proper spectral renderer.
using Spectrum = Vector3;

inline Spectrum make_zero_spectrum() {
    return Vector3{0, 0, 0};
}

inline Spectrum make_const_spectrum(Real v) {
    return Vector3{v, v, v};
}

inline Spectrum fromRGB(const Vector3 &rgb) {
    return rgb;
}

inline Spectrum sqrt(const Spectrum &s) {
    return Vector3{sqrt(max(s[0], Real(0))),
                   sqrt(max(s[1], Real(0))),
                   sqrt(max(s[2], Real(0)))};
}

inline Spectrum exp(const Spectrum &s) {
    return Vector3{exp(s[0]), exp(s[1]), exp(s[2])};
}

inline Real luminance(const Spectrum &s) {
    return s.x * Real(0.212671) + s.y * Real(0.715160) + s.z * Real(0.072169);
}

inline Real avg(const Spectrum &s) {
    return (s.x + s.y + s.z) / 3;
}

inline Vector3 toRGB(const Spectrum &s) {
    return s;
}

/// To support spectral data, we need to convert spectral measurements (how much energy at each wavelength) to
/// RGB. To do this, we first convert the spectral data to CIE XYZ, by
/// integrating over the XYZ response curve. Here we use an analytical response
/// curve proposed by Wyman et al.: https://jcgt.org/published/0002/02/01/
inline Real xFit_1931(Real wavelength) {
    Real t1 = (wavelength - Real(442.0)) * ((wavelength < Real(442.0)) ? Real(0.0624) : Real(0.0374));
    Real t2 = (wavelength - Real(599.8)) * ((wavelength < Real(599.8)) ? Real(0.0264) : Real(0.0323));
    Real t3 = (wavelength - Real(501.1)) * ((wavelength < Real(501.1)) ? Real(0.0490) : Real(0.0382));
    return Real(0.362) * exp(-Real(0.5) * t1 * t1) + 
           Real(1.056) * exp(-Real(0.5) * t2 * t2) -
           Real(0.065) * exp(-Real(0.5) * t3 * t3);
}
inline Real yFit_1931(Real wavelength) {
    Real t1 = (wavelength - Real(568.8)) * ((wavelength < Real(568.8)) ? Real(0.0213) : Real(0.0247));
    Real t2 = (wavelength - Real(530.9)) * ((wavelength < Real(530.9)) ? Real(0.0613) : Real(0.0322));
    return Real(0.821) * exp(-Real(0.5) * t1 * t1) +
           Real(0.286) * exp(-Real(0.5) * t2 * t2);
}
inline Real zFit_1931(Real wavelength) {
    Real t1 = (wavelength - Real(437.0)) * ((wavelength < Real(437.0)) ? Real(0.0845) : Real(0.0278));
    Real t2 = (wavelength - Real(459.0)) * ((wavelength < Real(459.0)) ? Real(0.0385) : Real(0.0725));
    return Real(1.217) * exp(-Real(0.5) * t1 * t1) +
           Real(0.681) * exp(-Real(0.5) * t2 * t2);
}
inline Vector3 XYZintegral_coeff(Real wavelength) {
    return Vector3{xFit_1931(wavelength), yFit_1931(wavelength), zFit_1931(wavelength)};
}

inline Vector3 integrate_XYZ(const std::vector<std::pair<Real, Real>> &data) {
    static const Real CIE_Y_integral = 106.856895;
    static const Real wavelength_beg = 400;
    static const Real wavelength_end = 700;
    if (data.size() == 0) {
        return Vector3{0, 0, 0};
    }
    Vector3 ret = Vector3{0, 0, 0};
    int data_pos = 0;
    // integrate from wavelength 400 nm to 700 nm, increment by 1nm at a time
    // linearly interpolate from the data
    for (Real wavelength = wavelength_beg; wavelength <= wavelength_end; wavelength += Real(1)) {
        // assume the spectrum data is sorted by wavelength
        // move data_pos such that wavelength is between two data or at one end
        while(data_pos < (int)data.size() - 1 &&
               !((data[data_pos].first <= wavelength &&
                  data[data_pos + 1].first > wavelength) ||
                 data[0].first > wavelength)) {
            data_pos += 1;
        }
        Real measurement = 0;
        if (data_pos < (int)data.size() - 1 && data[0].first <= wavelength) {
            Real curr_data = data[data_pos].second;
            Real next_data = data[std::min(data_pos + 1, (int)data.size() - 1)].second;
            Real curr_wave = data[data_pos].first;
            Real next_wave = data[std::min(data_pos + 1, (int)data.size() - 1)].first;
            // linearly interpolate
            measurement = curr_data * (next_wave - wavelength) / (next_wave - curr_wave) +
                          next_data * (wavelength - curr_wave) / (next_wave - curr_wave);
        } else {
            // assign the endpoint
            measurement = data[data_pos].second;
        }
        Vector3 coeff = XYZintegral_coeff(wavelength);
        ret += coeff * measurement;
    }
    Real wavelength_span = wavelength_end - wavelength_beg;
    ret *= (wavelength_span / (CIE_Y_integral * (wavelength_end - wavelength_beg)));
    return ret;
}

inline Vector3 XYZ_to_RGB(const Vector3 &xyz) {
    return Vector3{
        Real( 3.240479) * xyz[0] - Real(1.537150) * xyz[1] - Real(0.498535) * xyz[2],
        Real(-0.969256) * xyz[0] + Real(1.875991) * xyz[1] + Real(0.041556) * xyz[2],
        Real( 0.055648) * xyz[0] - Real(0.204043) * xyz[1] + Real(1.057311) * xyz[2]};
}

inline Vector3 sRGB_to_RGB(const Vector3 &srgb) {
    // https://en.wikipedia.org/wiki/SRGB#From_sRGB_to_CIE_XYZ
    Vector3 rgb = srgb;
    for (int i = 0; i < 3; i++) {
        rgb[i] = rgb[i] <= Real(0.04045) ?
            rgb[i] / Real(12.92) :
            pow((rgb[i] + Real(0.055)) / Real(1.055), Real(2.4));
    }
    return rgb;
}
