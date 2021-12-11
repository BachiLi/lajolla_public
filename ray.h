#pragma once

#include "lajolla.h"
#include "vector.h"

struct Ray {
    Vector3 org, dir;
    Real tnear, tfar;
};
