#pragma once

#include "lajolla.h"
#include "frame.h"
#include "ray.h"
#include "spectrum.h"
#include "vector.h"

#include <optional>

struct Scene;

/// An "PathVertex" represents a vertex of a light path.
/// We store the information we need for computing any sort of path contribution & sampling density.
struct PathVertex {
    Vector3 position;
    Vector3 geometric_normal; // always face at the same direction at shading_frame.n
    Frame shading_frame;
    Vector2 st; // A 2D parametrization of the surface. Irrelavant to UV mapping.
                // for triangle this is the barycentric coordinates, which we use
                // for interpolating the uv map.
    Vector2 uv; // The actual UV we use for texture fetching.
    // For texture filtering, stores approximatedly min(abs(du/dx), abs(dv/dx), abs(du/dy), abs(dv/dy))
    Real uv_screen_size;
    Real mean_curvature; // For ray differential propagation.
    Real ray_radius; // For ray differential propagation.
    int shape_id = -1;
    int primitive_id = -1; // For triangle meshes. This indicates which triangle it hits.
    int material_id = -1;

    // If the path vertex is inside a medium, these two IDs
    // are the same.
    int interior_medium_id = -1;
    int exterior_medium_id = -1;
};

/// Intersect a ray with a scene. If the ray doesn't hit anything,
/// returns an invalid optional output. 
std::optional<PathVertex> intersect(const Scene &scene,
                                    const Ray &ray,
                                    const RayDifferential &ray_diff = RayDifferential{});

/// Test is a ray segment intersect with anything in a scene.
bool occluded(const Scene &scene, const Ray &ray);

/// Computes the emission at a path vertex v, with the viewing direction
/// pointing outwards of the intersection.
Spectrum emission(const PathVertex &v,
                  const Vector3 &view_dir,
                  const Scene &scene);
