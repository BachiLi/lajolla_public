#pragma once

#include "lajolla.h"
#include "vector.h"

struct PathVertex;

/// Your typical Ray data structure!
struct Ray {
    Vector3 org, dir;
    Real tnear, tfar;
};

/// We adopt an approach for ray differentials
/// that is used in Renderman.
/// See Section 6.6 in "RenderMan: 
/// An Advanced Path Tracing Architecture for Movie Rendering"
/// https://graphics.pixar.com/library/RendermanTog2018/paper.pdf
/// The idea is to simplify Igehy's ray differential by only
/// storing two quantities: a "radius" that describes positional
/// differential, and a "spread" that describes directional differential.

/// For glossy/diffuse surfaces, Renderman used a heuristics based on the
/// PDF of the sampling direction and use larger spread for low PDF.
/// Here we use an even simpler heuristics: we linearly blend
/// between the specular spread and a constant based on roughness.
struct RayDifferential {
    // Radius is approximately (length(dp/dx) + length(dp/dy)) / 2
    // Spread is approximately (length(dd/dx) + length(dd/dy)) / 2
    // p is ray position, d is ray direction.
    Real radius = 0, spread = 0; // The units are pixels.
};

/// These functions propagate the ray differential information.
inline RayDifferential init_ray_differential(int w, int h) {
    return RayDifferential{Real(0), Real(0.25) / max(w, h)};
}

/// Update the radius (dp/dx) of a ray differential by propagating it over a distance.
inline Real transfer(const RayDifferential &r, Real dist) {
    return r.radius + r.spread * dist;
}

/// Update the spread (dd/dx) of a ray differential by scattering over a reflective surface.
inline Real reflect(const RayDifferential &r,
                    Real mean_curvature,
                    Real roughness) {
    Real spec_spread = r.spread + 2 * mean_curvature * r.radius;
    Real diff_spread = Real(0.2);
    return fmax(spec_spread * (1 - roughness) + diff_spread * roughness, Real(0));
}

/// Update the spread (dd/dx) of a ray differential by scattering over a refractive surface.
/// The Renderman reference https://graphics.pixar.com/library/RendermanTog2018/paper.pdf
/// did not specify how they propagate ray differentials through the refraction.
/// We simply guess a formula here: when eta == 1, the spread & radius should not change.
/// High eta makes ray more concentrated and reduces the spread, and vice versa.
/// so we simply divide everything by eta.
inline Real refract(const RayDifferential &r,
                    Real mean_curvature,
                    Real eta,
                    Real roughness) {
    Real spec_spread = (r.spread + 2 * mean_curvature * r.radius) / eta;
    Real diff_spread = Real(0.2);
    return fmax(spec_spread * (1 - roughness) + diff_spread * roughness, Real(0));
}
