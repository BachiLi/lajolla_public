#pragma once

#include "lajolla.h"
#include "vector.h"
#include <embree3/rtcore.h>

struct Shape {
    virtual uint32_t register_embree(const RTCDevice &device, const RTCScene &scene) const = 0;
};

struct Sphere : public Shape {
    Sphere(const Vector3 &position, Real radius) : position(position), radius(radius) {}

    uint32_t register_embree(const RTCDevice &device, const RTCScene &scene) const override;

    Vector3 position;
    Real radius;
};
