#include "camera.h"
#include "image.h"
#include "ray.h"
#include "vector.h"

#include <vector>

int main(int argc, char *argv[]) {
	int w = 256, h = 256;
	Image3 img(w, h);
	Camera cam(Matrix4x4::identity(),
		       Real(45),
		       w, h);
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			Ray ray = sample_primary(cam, Vector2((x + Real(0.5)) / w, (y + Real(0.5)) / h));
			img(x, y) = Vector3(ray.dir[0], ray.dir[1], ray.dir[2]);
		}
	}
	imwrite("out.pfm", img);
	return 0;
}