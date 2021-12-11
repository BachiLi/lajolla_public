#pragma once

#include "lajolla.h"
#include "vector.h"

struct Scene;
struct Ray;

struct Intersection {
    Vector3 position;
    Vector3 geometry_normal;
};

bool intersect(const Scene &scene, const Ray &ray, Intersection *isect);
