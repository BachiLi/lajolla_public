#pragma once

#include "vector.h"

/// Row-major matrix
template <typename T>
struct TMatrix4x4 {
    TMatrix4x4() {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                data[i][j] = T(0);
            }
        }
    }

    template <typename T2>
    TMatrix4x4(const T2 *arr) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                data[i][j] = (T)arr[i * 4 + j];
            }
        }
    }

    template <typename T2>
    TMatrix4x4(const TMatrix4x4<T2> &m) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                data[i][j] = T(m.data[i][j]);
            }
        }
    }

    template <typename T2>
    TMatrix4x4(T2 v00, T2 v01, T2 v02, T2 v03,
               T2 v10, T2 v11, T2 v12, T2 v13,
               T2 v20, T2 v21, T2 v22, T2 v23,
               T2 v30, T2 v31, T2 v32, T2 v33) {
        data[0][0] = (T)v00;
        data[0][1] = (T)v01;
        data[0][2] = (T)v02;
        data[0][3] = (T)v03;
        data[1][0] = (T)v10;
        data[1][1] = (T)v11;
        data[1][2] = (T)v12;
        data[1][3] = (T)v13;
        data[2][0] = (T)v20;
        data[2][1] = (T)v21;
        data[2][2] = (T)v22;
        data[2][3] = (T)v23;
        data[3][0] = (T)v30;
        data[3][1] = (T)v31;
        data[3][2] = (T)v32;
        data[3][3] = (T)v33;
    }

    const T& operator()(int i, int j) const {
        return data[i][j];
    }

    T& operator()(int i, int j) {
        return data[i][j];
    }

    static TMatrix4x4<T> identity() {
        TMatrix4x4<T> m(1, 0, 0, 0,
                        0, 1, 0, 0,
                        0, 0, 1, 0,
                        0, 0, 0, 1);
        return m;
    }

    T data[4][4];
};

using Matrix4x4 = TMatrix4x4<Real>;
using Matrix4x4f = TMatrix4x4<float>;

template <typename T>
TMatrix4x4<T> inverse(const TMatrix4x4<T> &m) {
    // https://stackoverflow.com/questions/1148309/inverting-a-4x4-matrix
    TMatrix4x4<T> inv;

    inv(0, 0) = m(1, 1) * m(2, 2) * m(3, 3) -
                m(1, 1) * m(2, 3) * m(3, 2) -
                m(2, 1) * m(1, 2) * m(3, 3) +
                m(2, 1) * m(1, 3) * m(3, 2) +
                m(3, 1) * m(1, 2) * m(2, 3) -
                m(3, 1) * m(1, 3) * m(2, 2);

    inv(1, 0) = -m(1, 0) * m(2, 2) * m(3, 3) +
                 m(1, 0) * m(2, 3) * m(3, 2) +
                 m(2, 0) * m(1, 2) * m(3, 3) -
                 m(2, 0) * m(1, 3) * m(3, 2) -
                 m(3, 0) * m(1, 2) * m(2, 3) +
                 m(3, 0) * m(1, 3) * m(2, 2);

    inv(2, 0) = m(1, 0) * m(2, 1) * m(3, 3) -
                m(1, 0) * m(2, 3) * m(3, 1) -
                m(2, 0) * m(1, 1) * m(3, 3) +
                m(2, 0) * m(1, 3) * m(3, 1) +
                m(3, 0) * m(1, 1) * m(2, 3) -
                m(3, 0) * m(1, 3) * m(2, 1);

    inv(3, 0) = -m(1, 0) * m(2, 1) * m(3, 2) +
                 m(1, 0) * m(2, 2) * m(3, 1) +
                 m(2, 0) * m(1, 1) * m(3, 2) -
                 m(2, 0) * m(1, 2) * m(3, 1) -
                 m(3, 0) * m(1, 1) * m(2, 2) +
                 m(3, 0) * m(1, 2) * m(2, 1);

    inv(0, 1) = -m(0, 1) * m(2, 2) * m(3, 3) +
                 m(0, 1) * m(2, 3) * m(3, 2) +
                 m(2, 1) * m(0, 2) * m(3, 3) -
                 m(2, 1) * m(0, 3) * m(3, 2) -
                 m(3, 1) * m(0, 2) * m(2, 3) +
                 m(3, 1) * m(0, 3) * m(2, 2);

    inv(1, 1) = m(0, 0) * m(2, 2) * m(3, 3) -
                m(0, 0) * m(2, 3) * m(3, 2) -
                m(2, 0) * m(0, 2) * m(3, 3) +
                m(2, 0) * m(0, 3) * m(3, 2) +
                m(3, 0) * m(0, 2) * m(2, 3) -
                m(3, 0) * m(0, 3) * m(2, 2);

    inv(2, 1) = -m(0, 0) * m(2, 1) * m(3, 3) +
                 m(0, 0) * m(2, 3) * m(3, 1) +
                 m(2, 0) * m(0, 1) * m(3, 3) -
                 m(2, 0) * m(0, 3) * m(3, 1) -
                 m(3, 0) * m(0, 1) * m(2, 3) +
                 m(3, 0) * m(0, 3) * m(2, 1);

    inv(3, 1) = m(0, 0) * m(2, 1) * m(3, 2) -
                m(0, 0) * m(2, 2) * m(3, 1) -
                m(2, 0) * m(0, 1) * m(3, 2) +
                m(2, 0) * m(0, 2) * m(3, 1) +
                m(3, 0) * m(0, 1) * m(2, 2) -
                m(3, 0) * m(0, 2) * m(2, 1);

    inv(0, 2) = m(0, 1) * m(1, 2) * m(3, 3) -
                m(0, 1) * m(1, 3) * m(3, 2) -
                m(1, 1) * m(0, 2) * m(3, 3) +
                m(1, 1) * m(0, 3) * m(3, 2) +
                m(3, 1) * m(0, 2) * m(1, 3) -
                m(3, 1) * m(0, 3) * m(1, 2);

    inv(1, 2) = -m(0, 0) * m(1, 2) * m(3, 3) +
                 m(0, 0) * m(1, 3) * m(3, 2) +
                 m(1, 0) * m(0, 2) * m(3, 3) -
                 m(1, 0) * m(0, 3) * m(3, 2) -
                 m(3, 0) * m(0, 2) * m(1, 3) +
                 m(3, 0) * m(0, 3) * m(1, 2);

    inv(2, 2) = m(0, 0) * m(1, 1) * m(3, 3) -
                m(0, 0) * m(1, 3) * m(3, 1) -
                m(1, 0) * m(0, 1) * m(3, 3) +
                m(1, 0) * m(0, 3) * m(3, 1) +
                m(3, 0) * m(0, 1) * m(1, 3) -
                m(3, 0) * m(0, 3) * m(1, 1);

    inv(3, 2) = -m(0, 0) * m(1, 1) * m(3, 2) +
                 m(0, 0) * m(1, 2) * m(3, 1) +
                 m(1, 0) * m(0, 1) * m(3, 2) -
                 m(1, 0) * m(0, 2) * m(3, 1) -
                 m(3, 0) * m(0, 1) * m(1, 2) +
                 m(3, 0) * m(0, 2) * m(1, 1);

    inv(0, 3) = -m(0, 1) * m(1, 2) * m(2, 3) +
                 m(0, 1) * m(1, 3) * m(2, 2) +
                 m(1, 1) * m(0, 2) * m(2, 3) -
                 m(1, 1) * m(0, 3) * m(2, 2) -
                 m(2, 1) * m(0, 2) * m(1, 3) +
                 m(2, 1) * m(0, 3) * m(1, 2);

    inv(1, 3) = m(0, 0) * m(1, 2) * m(2, 3) -
                m(0, 0) * m(1, 3) * m(2, 2) -
                m(1, 0) * m(0, 2) * m(2, 3) +
                m(1, 0) * m(0, 3) * m(2, 2) +
                m(2, 0) * m(0, 2) * m(1, 3) -
                m(2, 0) * m(0, 3) * m(1, 2);

    inv(2, 3) = -m(0, 0) * m(1, 1) * m(2, 3) +
                 m(0, 0) * m(1, 3) * m(2, 1) +
                 m(1, 0) * m(0, 1) * m(2, 3) -
                 m(1, 0) * m(0, 3) * m(2, 1) -
                 m(2, 0) * m(0, 1) * m(1, 3) +
                 m(2, 0) * m(0, 3) * m(1, 1);

    inv(3, 3) = m(0, 0) * m(1, 1) * m(2, 2) -
                m(0, 0) * m(1, 2) * m(2, 1) -
                m(1, 0) * m(0, 1) * m(2, 2) +
                m(1, 0) * m(0, 2) * m(2, 1) +
                m(2, 0) * m(0, 1) * m(1, 2) -
                m(2, 0) * m(0, 2) * m(1, 1);

    auto det = m(0, 0) * inv(0, 0) +
               m(0, 1) * inv(1, 0) +
               m(0, 2) * inv(2, 0) +
               m(0, 3) * inv(3, 0);

    if (det == 0) {
        return TMatrix4x4<T>{};
    }

    auto inv_det = 1.0 / det;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            inv(i, j) *= inv_det;
        }
    }

    return inv;
}

template <typename T>
inline TMatrix4x4<T> operator*(const TMatrix4x4<T> &m0, const TMatrix4x4<T> &m1) {
    TMatrix4x4<T> ret;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            ret(i, j) = T(0);
            for (int k = 0; k < 4; k++) {
                ret(i, j) += m0(i, k) * m1(k, j);
            }
        }
    }
    return ret;
}

template <typename T>
inline std::ostream& operator<<(std::ostream &os, const TMatrix4x4<T> &m) {
    return os << "[[" << m(0, 0) << ", " << m(0, 1) << ", " << m(0, 2) << ", " << m(0, 3) << "]," << std::endl <<
                  "[" << m(1, 0) << ", " << m(1, 1) << ", " << m(1, 2) << ", " << m(1, 3) << "]," << std::endl <<
                  "[" << m(2, 0) << ", " << m(2, 1) << ", " << m(2, 2) << ", " << m(2, 3) << "]," << std::endl <<
                  "[" << m(3, 0) << ", " << m(3, 1) << ", " << m(3, 2) << ", " << m(3, 3) << "]]" << std::endl;
}
