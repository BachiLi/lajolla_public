#include "image.h"
#include "vector.h"

#include <vector>

int main(int argc, char *argv[]) {
	int w = 256, h = 256;
	Image3 img(w, h);
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			img(x, y) = Vector3((Real)x / w, (Real)y / w, Real(0));
		}
	}
	imwrite("out.pfm", img);
	return 0;
}