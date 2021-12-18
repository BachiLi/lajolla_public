#pragma once

#include "lajolla.h"
#include "frame.h"
#include "spectrum.h"
#include "vector.h"

#include <optional>

struct Ray;
struct Scene;

/// An "PathVertex" represents a vertex of a light path.
/// We store the information we need for computing any sort of path contribution & sampling density.
struct PathVertex {
    Vector3 position;
    Vector3 geometry_normal;
    Frame shading_frame;
    Vector2 st; // A 2D parametrization of the surface. Irrelavant to UV mapping.
                // for triangle this is the barycentric coordinates, which we use
                // for interpolating the uv map.
    Vector2 uv; // The actual UV we use for texture fetching.
    Real mean_curvature; // For ray differential propagation.
    int shape_id = -1;
    int primitive_id = -1; // For triangle meshes. This indicates which triangle it hits.
    int material_id = -1;
};

/// Intersect a ray with a scene. If the ray doesn't hit anything,
/// returns an invalid optional output. 
std::optional<PathVertex> intersect(const Scene &scene, const Ray &ray);

/// Test is a ray segment intersect with anything in a scene.
bool occluded(const Scene &scene, const Ray &ray);

/// Computes the emission at a path vertex v, with the viewing direction
/// pointing outwards of the intersection.
Spectrum emission(const PathVertex &v,
                  const Vector3 &view_dir,
                  Real view_footprint,
                  const Scene &scene);
