#include "../frame.h"
#include <cstdio>

int main(int argc, char *argv[]) {
    Frame f(normalize(Vector3{0.3, 0.4, 0.5}));
    Vector3 v = Vector3{-1, -2, -3};
    Vector3 local_v = to_local(f, v);
    Vector3 world_v = to_world(f, local_v);
    if (distance(v, world_v) > Real(1e-3)) {
        printf("FAIL\n");
        return 1;
    }

    printf("SUCCESS\n");
    return 0;
}
