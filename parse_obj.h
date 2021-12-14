#pragma once

#include "lajolla.h"
#include "matrix.h"
#include "shape.h"
#include <string>

TriangleMesh parse_obj(const fs::path &filename, const Matrix4x4 &to_world);
