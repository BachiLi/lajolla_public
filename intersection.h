#pragma once

#include "lajolla.h"
#include "material.h"
#include "shape.h"
#include "vector.h"

struct Ray;
struct Scene;

struct Intersection {
    Vector3 position;
    Vector3 geometry_normal;
    const Shape *shape = nullptr;
    const Material *material = nullptr;
};

bool intersect(const Scene &scene, const Ray &ray, Intersection *isect);
