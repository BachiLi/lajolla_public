#pragma once

#include "lajolla.h"
#include "material.h"
#include "vector.h"

struct Scene;
struct Ray;

struct Intersection {
    Vector3 position;
    Vector3 geometry_normal;
    const Material *material = nullptr;
};

bool intersect(const Scene &scene, const Ray &ray, Intersection *isect);
