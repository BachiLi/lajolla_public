#pragma once

#include "lajolla.h"
#include "vector.h"
#include <vector>

// Numerical robust computation of angle between unit vectors
inline Real unit_angle(const Vector3 &u, const Vector3 &v) {
    if (dot(u, v) < 0)
        return (c_PI - 2) * asin(Real(0.5) * length(v + u));
    else
        return 2 * asin(Real(0.5) * length(v - u));
}

inline std::vector<Vector3> compute_normal(const std::vector<Vector3> &vertices,
                                           const std::vector<Vector3i> &indices) {
	std::vector<Vector3> normals(vertices.size(), Vector3{0, 0, 0});

    // Nelson Max, "Computing Vertex Normals from Facet Normals", 1999
    for (auto &index : indices) {
        Vector3 n = Vector3{0, 0, 0};
        for (int i = 0; i < 3; ++i) {
            const Vector3 &v0 = vertices[index[i]];
            const Vector3 &v1 = vertices[index[(i + 1) % 3]];
            const Vector3 &v2 = vertices[index[(i + 2) % 3]];
            Vector3 side1 = v1 - v0, side2 = v2 - v0;
            if (i == 0) {
                n = cross(side1, side2);
                Real l = length(n);
                if (l == 0) {
                    break;
                }
                n = n / l;
            }
            Real angle = unit_angle(normalize(side1), normalize(side2));
            normals[index[i]] = normals[index[i]] + n * angle;
        }
    }

    for (auto &n : normals) {
        Real l = length(n);
        if (l != 0) {
            n = n / l;
        } else {
        	// degenerate normals, set it to 0
            n = Vector3{0, 0, 0};
        }
    }
    return normals;
}
