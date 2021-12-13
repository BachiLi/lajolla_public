#pragma once

#include "lajolla.h"
#include "matrix.h"
#include "shape.h"
#include <string>

TriangleMesh parse_obj(const std::string &filename, const Matrix4x4 &to_world);
