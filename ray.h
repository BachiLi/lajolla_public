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
    Real radius = 0, spread = 0; // The units are pixels.
};

/// These functions propagate the ray differential information.
inline RayDifferential init_ray_differential() {
    return RayDifferential{Real(0), Real(0.25)};
}

inline RayDifferential transfer(const RayDifferential &r,
                                Real dist) {
    return RayDifferential{r.radius + r.spread * dist, r.spread};
}

inline RayDifferential reflect(const RayDifferential &r,
                               Real mean_curvature,
                               Real roughness) {
    Real spec_spread = r.spread + 2 * mean_curvature * r.radius;
    Real diff_spread = Real(0.2);
    return RayDifferential{r.radius,
        fmax(spec_spread * (1 - roughness) + diff_spread * roughness, Real(0))};
}

/// The Renderman reference https://graphics.pixar.com/library/RendermanTog2018/paper.pdf
/// did not specify how they propagate ray differentials through the refraction.
/// We simply guess a formula here: when eta == 1, the spread & radius should not change.
inline RayDifferential refract(const RayDifferential &r,
                               Real mean_curvature,
                               Real eta,
                               Real roughness) {
    Real spec_spread = eta * r.spread + 2 * (eta - 1) * (eta - 1) * mean_curvature * r.radius;
    Real diff_spread = Real(0.2);
    return RayDifferential{r.radius,
        fmax(spec_spread * (1 - roughness) + diff_spread * roughness, Real(0))};
}
