#include "../image.h"
#include "../mipmap.h"
#include <cstdio>

int main(int argc, char *argv[]) {
    Image3 img(64, 64);
    for (int i = 0; i < 64 * 64; i++) {
        img(i) = Vector3{1, 1, 1};
    }
    Mipmap3 mipmap = make_mipmap(img);
    for (int l = 0; l < (int)mipmap.images.size(); l++) {
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 64; x++) {
                Vector3 v = lookup(mipmap,
                    (x + Real(0.5)) / 64,
                    (y + Real(0.5)) / 64,
                    l + Real(0.5));
                if (fabs(v.x - 1) > Real(1e-3) ||
                        fabs(v.y - 1) > Real(1e-3) ||
                        fabs(v.z - 1) > Real(1e-3)) {
                    printf("FAIL\n");
                    return 1;
                }
            }
        }
    }

    printf("SUCCESS\n");
    return 0;
}
