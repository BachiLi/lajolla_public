#include "render.h"
#include "intersection.h"
#include "material.h"
#include "pcg.h"
#include "scene.h"

std::shared_ptr<Image3> render(const Scene &scene) {
    int w = scene.camera.width, h = scene.camera.height;
    std::shared_ptr<Image3> img_ = std::make_shared<Image3>(w, h);
    Image3 &img = *img_;
    pcg32_state rng = init_pcg32();
    int spp = 32;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Spectrum radiance = make_zero_spectrum();
            for (int s = 0; s < spp; s++) {
                Ray ray = sample_primary(scene.camera, Vector2((x + Real(0.5)) / w, (y + Real(0.5)) / h));
                Intersection isect;
                if (intersect(scene, ray, &isect)) {
                    Vector3 normal = isect.geometry_normal;
                    Vector2 light_uv{next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng)};
                    Real light_w = next_pcg32_real<Real>(rng);
                    int light_id = sample_light(scene, light_w);
                    const Light &light = scene.lights[light_id];
                    LightSample ls = sample_point_on_light(light, light_uv, scene);
                    const ShapeSample &ss = ls.shape_sample;
                    Vector3 dir_light = normalize(ss.position - isect.position);
                    Vector3 dir_view = -ray.dir;
                    Real geometry_term = fmax(-dot(dir_light, ss.geometry_normal), Real(0)) /
                        distance_squared(ss.position, isect.position);
                    Real light_pdf = light_pmf(scene, light_id) * pdf_point_on_light(light, scene);
                    assert(isect.material != nullptr);
                    Spectrum brdf = eval(*isect.material, dir_light, dir_view, isect);
                    radiance += (ls.intensity * brdf * geometry_term / light_pdf);
                }
            }
            img(x, y) = radiance / Real(spp);
        }
    }
    return img_;
}
