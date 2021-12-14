#pragma once

#include "lajolla.h"
#include "frame.h"
#include "material.h"
#include "shape.h"
#include "vector.h"

struct Ray;
struct Scene;

struct Intersection {
    Vector3 position;
    Vector3 geometry_normal;
    Frame shading_frame;
    int shape_id;
    const Shape *shape = nullptr;
    const Material *material = nullptr;
};

bool intersect(const Scene &scene, const Ray &ray, Intersection *isect);
bool occluded(const Scene &scene, const Ray &ray);
// Computes the emission at Intersection isect, with the viewing direction
// pointing outwards of the intersection.
Spectrum emission(const Intersection &isect, const Vector3 &view_dir, const Scene &scene);
