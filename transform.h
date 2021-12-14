#pragma once

#include "lajolla.h"
#include "matrix.h"
#include "vector.h"

/// A collection of 3D transformations.
Matrix4x4 translate(const Vector3 &delta);
Matrix4x4 scale(const Vector3 &scale);
Matrix4x4 rotate(Real angle, const Vector3 &axis);
Matrix4x4 look_at(const Vector3 &pos, const Vector3 &look, const Vector3 &up);
Matrix4x4 perspective(Real fov);
/// Actually transform the vectors given a transformation.
Vector3 xform_point(const Matrix4x4 &xform, const Vector3 &pt);
Vector3 xform_vector(const Matrix4x4 &xform, const Vector3 &vec);
Vector3 xform_normal(const Matrix4x4 &inv_xform, const Vector3 &n);
