#include "transform.h"

/// Much of the code is taken from pbrt https://github.com/mmp/pbrt-v3/tree/master/src

Matrix4x4 translate(const Vector3 &delta) {
    return Matrix4x4(Real(1), Real(0), Real(0), delta[0],
                     Real(0), Real(1), Real(0), delta[1],
                     Real(0), Real(0), Real(1), delta[2],
                     Real(0), Real(0), Real(0), Real(1));
}

Matrix4x4 scale(const Vector3 &s) {
    return Matrix4x4(   s[0], Real(0), Real(0), Real(0),
                     Real(0),    s[1], Real(0), Real(0),
                     Real(0), Real(0),    s[2], Real(0),
                     Real(0), Real(0), Real(0), Real(1));
}

Matrix4x4 rotate(Real angle, const Vector3 &axis) {
    Vector3 a = normalize(axis);
    Real s = sin(radians(angle));
    Real c = cos(radians(angle));
    Matrix4x4 m;
    m(0, 0) = a[0] * a[0] + (1 - a[0] * a[0]) * c;
    m(0, 1) = a[0] * a[1] * (1 - c) - a[2] * s;
    m(0, 2) = a[0] * a[2] * (1 - c) + a[1] * s;
    m(0, 3) = 0;

    m(1, 0) = a[0] * a[1] * (1 - c) + a[2] * s;
    m(1, 1) = a[1] * a[1] + (1 - a[1] * a[1]) * c;
    m(1, 2) = a[1] * a[2] * (1 - c) - a[0] * s;
    m(1, 3) = 0;

    m(2, 0) = a[0] * a[2] * (1 - c) - a[1] * s;
    m(2, 1) = a[1] * a[2] * (1 - c) + a[0] * s;
    m(2, 2) = a[2] * a[2] + (1 - a[2] * a[2]) * c;
    m(2, 3) = 0;

    m(3, 0) = 0;
    m(3, 1) = 0;
    m(3, 2) = 0;
    m(3, 3) = 1;
    return m;
}

Matrix4x4 look_at(const Vector3 &pos, const Vector3 &look, const Vector3 &up) {
    Matrix4x4 m;
    Vector3 dir = normalize(look - pos);
    assert(length(cross(normalize(up), dir)) != 0);
    Vector3 left = normalize(cross(normalize(up), dir));
    Vector3 new_up = cross(dir, left);
    m(0, 0) = left[0];
    m(1, 0) = left[1];
    m(2, 0) = left[2];
    m(3, 0) = 0;
    m(0, 1) = new_up[0];
    m(1, 1) = new_up[1];
    m(2, 1) = new_up[2];
    m(3, 1) = 0;
    m(0, 2) = dir[0];
    m(1, 2) = dir[1];
    m(2, 2) = dir[2];
    m(3, 2) = 0;
    m(0, 3) = pos[0];
    m(1, 3) = pos[1];
    m(2, 3) = pos[2];
    m(3, 3) = 1;
    return m;
}

Matrix4x4 perspective(Real fov) {
    Real cot = Real(1.0) / tan(radians(fov / 2.0));
    return Matrix4x4(    cot, Real(0), Real(0),  Real(0),
                     Real(0),     cot, Real(0),  Real(0),
                     Real(0), Real(0), Real(1), Real(-1),
                     Real(0), Real(0), Real(1),  Real(0));
}

Vector3 xform_point(const Matrix4x4 &xform, const Vector3 &pt) {
    Vector4 tpt(
        xform(0, 0) * pt[0] + xform(0, 1) * pt[1] + xform(0, 2) * pt[2] + xform(0, 3),
        xform(1, 0) * pt[0] + xform(1, 1) * pt[1] + xform(1, 2) * pt[2] + xform(1, 3),
        xform(2, 0) * pt[0] + xform(2, 1) * pt[1] + xform(2, 2) * pt[2] + xform(2, 3),
        xform(3, 0) * pt[0] + xform(3, 1) * pt[1] + xform(3, 2) * pt[2] + xform(3, 3));
    Real inv_w = Real(1) / tpt[3];
    return Vector3(tpt[0] * inv_w, tpt[1] * inv_w, tpt[2] * inv_w);
}

Vector3 xform_vector(const Matrix4x4 &xform, const Vector3 &vec) {
    return Vector3(xform(0, 0) * vec[0] + xform(0, 1) * vec[1] + xform(0, 2) * vec[2],
                   xform(1, 0) * vec[0] + xform(1, 1) * vec[1] + xform(1, 2) * vec[2],
                   xform(2, 0) * vec[0] + xform(2, 1) * vec[1] + xform(2, 2) * vec[2]);
}

Vector3 xform_normal(const Matrix4x4 &inv_xform, const Vector3 &n) {
    return normalize(Vector3{
        inv_xform(0, 0) * n[0] + inv_xform(1, 0) * n[1] + inv_xform(2, 0) * n[2],
        inv_xform(0, 1) * n[0] + inv_xform(1, 1) * n[1] + inv_xform(2, 1) * n[2],
        inv_xform(0, 2) * n[0] + inv_xform(1, 2) * n[1] + inv_xform(2, 2) * n[2]});
}
