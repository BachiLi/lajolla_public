#include "render.h"
#include "intersection.h"
#include "scene.h"

std::shared_ptr<Image3> render(const Scene &scene) {
    int w = scene.camera.width, h = scene.camera.height;
    std::shared_ptr<Image3> img_ = std::make_shared<Image3>(w, h);
    Image3 &img = *img_;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Ray ray = sample_primary(scene.camera, Vector2((x + Real(0.5)) / w, (y + Real(0.5)) / h));
            Intersection isect;
            if (intersect(scene, ray, &isect)) {
                Vector3 geometry_normal{isect.geometry_normal.x, isect.geometry_normal.y, isect.geometry_normal.z};
                geometry_normal = normalize(geometry_normal);
                img(x, y) = geometry_normal + Vector3{1, 1, 1};
            } else {
                img(x, y) = Vector3(0, 0, 0);
            }
        }
    }
    return img_;
}
