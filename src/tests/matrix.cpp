#include "../matrix.h"
#include <cstdio>

int main(int argc, char *argv[]) {
    Matrix4x4 m = Matrix4x4(
        Real(11), Real(2), Real(3), Real(4),
        Real(5), Real(16), Real(7), Real(8),
        Real(9), Real(10), Real(21), Real(12),
        Real(13), Real(14), Real(15), Real(26)
    );
    Matrix4x4 m_inv = inverse(m);
    Matrix4x4 m_inv_m = m_inv * m;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            Real target = i == j ? Real(1) : Real(0);
            if (fabs(m_inv_m(i, j) - target) > Real(1e-3)) {
                printf("FAIL\n");
                return 1;
            }
        }
    }

    printf("SUCCESS\n");
    return 0;
}
