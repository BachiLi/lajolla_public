#pragma once

#include "lajolla.h"
#include "matrix.h"
#include "point_and_normal.h"
#include "shape.h"
#include "spectrum.h"
#include "texture.h"
#include "vector.h"
#include <variant>

struct Scene;

/// An area light attached on a shape to make it emit lights.
struct DiffuseAreaLight {
    int shape_id;
    Vector3 intensity;
};

/// An environment map (Envmap) is an infinitely far area light source
/// that covers the whole bounding spherical domain of the scene.
/// A texture is used to represent light coming from each direction.
struct Envmap {
    Texture<Spectrum> values;
    Matrix4x4 to_world, to_local;
    Real scale;

    // For sampling a point on the envmap
    TableDist2D sampling_dist;
};

// To add more lights, first create a struct for the light, add it to the variant type below,
// then implement all the relevant function below with the Light type.
using Light = std::variant<DiffuseAreaLight, Envmap>;

/// Computes the total power the light emit to all positions and directions.
/// Useful for sampling.
Real light_power(const Light &light, const Scene &scene);

/// Given some random numbers and a reference point, sample a point on the light source.
/// If the point is on a surface, returns both the point & normal on it.
/// If the point is infinitely far away (e.g., on an environment map),
/// we store the direction that points towards the origin in PointAndNormal.normal.
/// rnd_param_w is usually used for choosing a discrete element e.g., choosing a triangle in a mesh light.
/// rnd_param_uv is usually used for picking a point on that element.
PointAndNormal sample_point_on_light(const Light &light,
                                     const Vector3 &ref_point,
                                     const Vector2 &rnd_param_uv,
                                     Real rnd_param_w,
                                     const Scene &scene);

/// Given a point on the light source and a reference point,
/// compute the sampling density for the function above.
Real pdf_point_on_light(const Light &light,
                        const PointAndNormal &point_on_light,
                        const Vector3 &ref_point,
                        const Scene &scene);

/// Given a viewing direction pointing outwards from the light, and a point on the light,
/// compute the emission of the light. We also need the "footprint" of the ray
/// for texture filtering. For finite position, view_footprint stores (approximatedly) du/dx
/// and for infinite direction (e.g., envmap), view_footprint stores the approximated ddir/dx.
Spectrum emission(const Light &light,
                  const Vector3 &view_dir,
                  Real view_footprint,
                  const PointAndNormal &point_on_light,
                  const Scene &scene);

/// Some lights require storing sampling data structures inside. This function initialize them.
void init_sampling_dist(Light &light, const Scene &scene);

inline bool is_envmap(const Light &light) {
    return std::get_if<Envmap>(&light) != nullptr;
}
