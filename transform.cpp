#include "transform.h"

inline Real radians(const Real deg) {
    return (c_PI / Real(180)) * deg;
}

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

Matrix4x4 perspective(const Real fov) {
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

Vector3 xform_vector(const Matrix4x4 &xform, const Vector3 &pt) {
    return Vector3(xform(0, 0) * pt[0] + xform(0, 1) * pt[1] + xform(0, 2) * pt[2],
                   xform(1, 0) * pt[0] + xform(1, 1) * pt[1] + xform(1, 2) * pt[2],
                   xform(2, 0) * pt[0] + xform(2, 1) * pt[1] + xform(2, 2) * pt[2]);
}
