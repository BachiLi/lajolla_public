#pragma once

#include "lajolla.h"
#include "vector.h"

/// Your typical Ray data structure!
struct Ray {
    Vector3 org, dir;
    Real tnear, tfar;
};
