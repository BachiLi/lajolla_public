#pragma once

#include "lajolla.h"
#include "spectrum.h"

/// A microfacet model assumes that the surface is composed of infinitely many little mirrors/glasses.
/// The orientation of the mirrors determines the amount of lights reflected.
/// The distribution of the orientation is determined empirically.
/// The distribution that fits the best to the data we have so far (which is not a lot of data)
/// is from Trowbridge and Reitz's 1975 paper "Average irregularity representation of a rough ray reflection",
/// wildly known as "GGX" (seems to stand for "Ground Glass X" https://twitter.com/CasualEffects/status/783018211130441728).
/// 
/// We will use a generalized version of GGX called Generalized Trowbridge and Reitz (GTR),
/// proposed by Brent Burley and folks at Disney (https://www.disneyanimation.com/publications/physically-based-shading-at-disney/)
/// as our normal distribution function. GTR2 is equivalent to GGX.

/// Schlick's Fresnel equation approximation
/// from "An Inexpensive BRDF Model for Physically-based Rendering", Schlick
/// https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.50.2297&rep=rep1&type=pdf
/// See "Memo on Fresnel equations" from Sebastien Lagarde
/// for a really nice introduction.
/// https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/
inline Spectrum schlick_fresnel(const Spectrum &F0, Real cos_theta) {
    return F0 + (Real(1) - F0) *
        pow(max(1 - cos_theta, Real(0)), Real(5));
}

/// Code from https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/
inline Real fresnel_dielectric(Real eta, Real cos_theta) {
    Real sin_theta_2 = 1 - cos_theta * cos_theta;
    Real scaled_sin_theta_2 = sin_theta_2 / (eta * eta);
    if (scaled_sin_theta_2 >= 1) {
        // total internal reflection
        return 1;
    }

    Real t0 = sqrt(1 - scaled_sin_theta_2);
    Real t1 = eta * t0;
    Real t2 = eta * cos_theta;

    Real rs = (cos_theta - t1) / (cos_theta + t1);
    Real rp = (t0 - t2) / (t0 + t2);

    return 0.5 * (rs * rs + rp * rp);
}

inline Real GTR2(Real n_dot_h, Real roughness) {
    Real alpha = roughness * roughness;
    Real a2 = alpha * alpha;
    Real t = 1 + (a2 - 1) * n_dot_h * n_dot_h;
    return a2 / (c_PI * t*t);
}

inline Real GGX(Real n_dot_h, Real roughness) {
    return GTR2(n_dot_h, roughness);
}

/// The masking term models the occlusion between
/// the small mirrors of the microfacet models.
/// See Eric Heitz's paper "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
/// for a great explanation.
/// https://jcgt.org/published/0003/02/03/paper.pdf
/// The derivation is based on Smith's paper "Geometrical shadowing of a random rough surface".
inline Real smith_masking(Real n_dot_v, Real roughness) {
    Real alpha = roughness * roughness;
    Real a = alpha * alpha;
    Real b = n_dot_v * n_dot_v;
    return Real(1) / (n_dot_v + sqrt(a + b - a * b));
}
