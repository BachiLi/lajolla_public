#include "../filter.h"
#include <cstdio>

Real compute_determinant(const Filter &f, const Vector2 &rnd_param) {
    Real eps = Real(1e-6);
    Vector2 s = sample(f, rnd_param);
    Vector2 s_u = sample(f, rnd_param + Vector2{eps, Real(0)});
    Vector2 s_v = sample(f, rnd_param + Vector2{Real(0), eps});
    Vector2 s_du = (s_u - s) / eps;
    Vector2 s_dv = (s_v - s) / eps;
    Real det = fabs(s_du.x * s_dv.y - s_du.y * s_dv.x);
    return det;
}

int main(int argc, char *argv[]) {
    Real width = 2;
    Vector2 rnd_param = Vector2{0.3, 0.4};
    {
        Filter f = Box{width};
        // The derivative of box sample w.r.t.
        // rnd_param will form a Jacobian.
        // The determinant of this Jacobian should be
        // a constant width * width (the inverse value of the normalized box filter kernel)
        Real det = compute_determinant(f, rnd_param);
        if (fabs(det - width * width) > Real(1e-3)) {
            printf("FAIL\n");
            return 1;
        }
    }

    // Do the same test for other filters
    {
        Filter f = Tent{width};
        Vector2 s = sample(f, rnd_param);
        Real det = compute_determinant(f, rnd_param);
        // For tent filter, the kernel is
        // (1 - abs(x) / half_width) / half_width *
        // (1 - abs(y) / half_width) / half_width
        Real half_width = width / 2;
        Real norm = half_width;
        Real kernel = ((1 - fabs(s.x) / half_width) / norm) *
                      ((1 - fabs(s.y) / half_width) / norm);
        Real inv_kernel = 1 / kernel;
        if (fabs(det - inv_kernel) > Real(1e-3)) {
            printf("FAIL\n");
            return 1;
        }
    }

    // Do the same test for other filters
    {
        Real stddev = width;
        Filter f = Gaussian{stddev};
        Vector2 s = sample(f, rnd_param);
        Real det = compute_determinant(f, rnd_param);
        // kernel is a gaussian
        Real kernel = exp(-((s.x * s.x + s.y * s.y) / (stddev * stddev)) / 2) /
            (stddev * stddev * 2 * c_PI);
        Real inv_kernel = 1 / kernel;
        if (fabs(det - inv_kernel) > Real(1e-3)) {
            printf("FAIL\n");
            return 1;
        }
    }

    printf("SUCCESS\n");
    return 0;
}
