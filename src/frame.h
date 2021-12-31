#pragma once

#include "lajolla.h"
#include "vector.h"

/// Given a vector n, outputs two vectors such that all three vectors are
/// orthogonal to each other.
/// The approach here is based on Frisvad's paper
/// "Building an Orthonormal Basis from a 3D Unit Vector Without Normalization"
/// https://backend.orbit.dtu.dk/ws/portalfiles/portal/126824972/onb_frisvad_jgt2012_v2.pdf
inline std::pair<Vector3, Vector3> coordinate_system(const Vector3 &n) {
    if (n[2] < Real(-1 + 1e-6)) {
        return std::make_pair(Vector3{0, -1, 0},
                              Vector3{-1, 0, 0});
    } else {
        Real a = 1 / (1 + n[2]);
        Real b = -n[0] * n[1] * a;
        return std::make_pair(Vector3{1 - n[0] * n[0] * a, b, -n[0]},
                              Vector3{b, 1 - n[1] * n[1] * a, -n[1]});
    }
}

/// A "Frame" is a coordinate basis that consists of three orthogonal unit vectors.
/// This is useful for sampling points on a hemisphere or defining anisotropic BSDFs.
struct Frame {
    Frame() {}

    Frame(const Vector3 &x, const Vector3 &y, const Vector3 &n)
        : x(x), y(y), n(n) {}

    Frame(const Vector3 &n) : n(n) {
        std::tie(x, y) = coordinate_system(n);
    }

    inline Vector3& operator[](int i) {
        return *(&x + i);
    }
    inline const Vector3& operator[](int i) const {
        return *(&x + i);
    }
    Vector3 x, y, n;
};

/// Flip the frame to opposite direction.
inline Frame operator-(const Frame &frame) {
    return Frame(-frame.x, -frame.y, -frame.n);
}

/// Project a vector to a frame's local coordinates.
inline Vector3 to_local(const Frame &frame, const Vector3 &v) {
    return Vector3{dot(v, frame[0]), dot(v, frame[1]), dot(v, frame[2])};
}

/// Convert a vector in a frame's local coordinates to the reference coordinate the frame is in.
inline Vector3 to_world(const Frame &frame, const Vector3 &v) {
    return frame[0] * v[0] + frame[1] * v[1] + frame[2] * v[2];
}

inline std::ostream& operator<<(std::ostream &os, const Frame &f) {
    return os << "Frame(" << f[0] << ", " << f[1] << ", " << f[2] << ")";
}
