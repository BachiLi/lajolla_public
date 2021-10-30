#pragma once

#include "lajolla.h"
#include "matrix.h"
#include "vector.h"

Matrix4x4 translate(const Vector3 &delta);
Matrix4x4 scale(const Vector3 &scale);
Matrix4x4 perspective(const Real fov);
Vector3 xform_point(const Matrix4x4 &xform, const Vector3 &pt);
Vector3 xform_vector(const Matrix4x4 &xform, const Vector3 &pt);
