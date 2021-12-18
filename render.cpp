#include "render.h"
#include "intersection.h"
#include "material.h"
#include "pcg.h"
#include "scene.h"

/// Render auxiliary buffers e.g., depth.
std::shared_ptr<Image3> aux_render(const Scene &scene) {
    int w = scene.camera.width, h = scene.camera.height;
    std::shared_ptr<Image3> img_ = std::make_shared<Image3>(w, h);
    Image3 &img = *img_;
    pcg32_state rng = init_pcg32();
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Spectrum radiance = make_zero_spectrum();
            Ray ray = sample_primary(scene.camera, Vector2((x + Real(0.5)) / w, (y + Real(0.5)) / h));
            if (std::optional<PathVertex> vertex = intersect(scene, ray)) {
                Real dist = distance(vertex->position, ray.org);
                Vector3 color{0, 0, 0};
                if (scene.options.integrator == Integrator::Depth) {
                    color = Vector3{dist, dist, dist};
                } else if (scene.options.integrator == Integrator::MeanCurvature) {
                    Real kappa = vertex->mean_curvature;
                    color = Vector3{kappa, kappa, kappa};
                } else if (scene.options.integrator == Integrator::RayDifferential) {
                    RayDifferential ray_diff = init_ray_differential();
                    ray_diff = transfer(ray_diff, distance(ray.org, vertex->position));
                    color = Vector3{ray_diff.radius, ray_diff.spread, Real(0)};
                } else if (scene.options.integrator == Integrator::MipmapLevel) {
                    RayDifferential ray_diff = init_ray_differential();
                    ray_diff = transfer(ray_diff, distance(ray.org, vertex->position));
                    const Material &mat = scene.materials[vertex->material_id];
                    const TextureSpectrum &texture = get_texture(mat);
                    auto *t = std::get_if<ImageTexture<Spectrum>>(&texture);
                    if (t != nullptr) {
                        const Mipmap3 &mipmap = get_img3(scene.texture_pool, t->texture_id);
                        Vector2 uv{modulo(vertex->uv[0] * t->uscale, Real(1)),
                                   modulo(vertex->uv[1] * t->vscale, Real(1))};
                        Real footprint = ray_diff.radius;
                        Real scaled_footprint = max(get_width(mipmap), get_height(mipmap)) *
                                                max(t->uscale, t->vscale) * footprint;
                        Real level = log2(max(footprint, Real(1e-8f)));
                        color = Vector3{level, level, level};
                    }
                }
                img(x, y) = color;
            } else {
                img(x, y) = Vector3{0, 0, 0};
            }
        }
    }
    return img_;
}

/// Unidirectional path tracing
Spectrum path_tracing(const Scene &scene,
                      int x, int y, /* pixel coordinates */
                      pcg32_state &rng) {
    int w = scene.camera.width, h = scene.camera.height;
    Vector2 screen_pos((x + next_pcg32_real<Real>(rng)) / w,
                       (y + next_pcg32_real<Real>(rng)) / h);
    Ray ray = sample_primary(scene.camera, screen_pos);
    RayDifferential ray_diff = init_ray_differential();

    std::optional<PathVertex> vertex_ = intersect(scene, ray);
    if (!vertex_) {
        // Hit background. Account for the environment map if needed.
        if (has_envmap(scene)) {
            const Light &envmap = get_envmap(scene);
            return emission(envmap,
                            -ray.dir, // pointing outwards from light
                            ray_diff.spread,
                            PointAndNormal{}, // dummy parameter for envmap
                            scene);
        }
        return make_zero_spectrum();
    }
    PathVertex vertex = *vertex_;
    ray_diff = transfer(ray_diff, distance(ray.org, vertex.position));

    Spectrum radiance = make_zero_spectrum();
    // A path's contribution is 
    // C(v) = W(v0, v1) * G(v0, v1) * f(v0, v1, v2) * 
    //                    G(v1, v2) * f(v1, v2, v3) * 
    //                  ........
    //                  * G(v_{n-1}, v_n) * L(v_{n-1}, v_n)
    // where v is the path vertices, W is the sensor response
    // G is the geometry term, f is the BSDF, L is the emission
    //
    // "sample_primary" importance samples both W and G,
    // and we assume it always has weight 1.

    // current path contrib stores the path contribution from 
    // v0 up to v_{i} (the BSDF f(v_{i-1}, v_i, v_{i+1}) is not included), 
    // where i is where the PathVertex "vertex" lies on.
    Spectrum current_path_contrib = fromRGB(Vector3{1, 1, 1});
    // current_path_pdf stores the probability density of 
    // computing this path v from v0 up to v_i,
    // so that we can compute the Monte Carlo estimates C/p. 
    Real current_path_pdf = Real(1);

    // We hit a light immediately. 
    // This path has only two vertices and has contribution
    // C = W(v0, v1) * G(v0, v1) * L(v0, v1)
    if (is_light(scene.shapes[vertex.shape_id])) {
        radiance += (current_path_contrib / current_path_pdf) *
            emission(vertex, -ray.dir, ray_diff.radius, scene);
    }

    // We iteratively sum up path contributions from paths with different number of vertices
    for (int num_vertices = 1; num_vertices <= scene.options.max_depth - 1; num_vertices++) {
        // We are at v_i, and all the path contribution on and before has been accounted for.
        // Now we need to somehow generate v_{i+1} to account for paths with more vertices.
        // In path tracing, we generate two vertices:
        // 1) we sample a point on the light source (often called "Next Event Estimation")
        // 2) we randomly trace a ray from the surface point at v_i and hope we hit something.
        //
        // The first importance samples L(v_i, v_{i+1}), and the second
        // importance samples f(v_{i-1}, v_i, v_{i+1}) * G(v_i, v_{i+1})
        //
        // We then combine the two sampling strategies to estimate the contribution using weighted average.
        // Say the contribution of the first sampling is C1 (with probability density p1), 
        // and the contribution of the second sampling is C2 (with probability density p2,
        // then we compute the estimate as w1*C1/p1 + w2*C2/p2.
        //
        // Assuming the vertices for C1 is v^1, and v^2 for C2,
        // Eric Veach showed that it is a good idea setting 
        // w1 = p_1(v^1)^k / (p_1(v^1)^k + p_2(v^1)^k)
        // w2 = p_2(v^2)^k / (p_1(v^2)^k + p_2(v^2)^k),
        // where k is some scalar real number, and p_a(v^b) is the probability density of generating
        // vertices v^b using sampling method "a".
        // We will set k=2 as suggested by Eric Veach.

        // Finally, we set our "next vertex" in the loop to the v_{i+1} generated
        // by the second sampling, and update current_path_contrib & current_pdf using
        // our hemisphere sampling.

        // Let's implement this!
        const Material &mat = scene.materials[vertex.material_id];

        // First, we sample a point on the light source.
        // We do this by first picking a light source, then pick a point on it.
        Vector2 light_uv{next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng)};
        Real light_w = next_pcg32_real<Real>(rng);
        Real shape_w = next_pcg32_real<Real>(rng);
        int light_id = sample_light(scene, light_w);
        const Light &light = scene.lights[light_id];
        LightSampleRecord lr = sample_point_on_light(light, light_uv, shape_w, scene);

        // Next, we compute w1*C1/p1. We store C1/p1 in C1.
        Spectrum C1 = make_zero_spectrum();
        Real w1 = 0;
        // Remember "current_path_contrib" already stores all the path contribution on and before v_i.
        // So we only need to compute G(v_{i}, v_{i+1}) * f(v_{i-1}, v_{i}, v_{i+1}) * L(v_{i}, v_{i+1})
        {
            const PointAndNormal &point_on_light = lr.point_on_light;
            // Let's first deal with C1 = G * f * L.
            // Let's first compute G.
            Real G = 0;
            Vector3 dir_light;
            // The geometry term is different between directional light sources and
            // others. Currently we only have environment maps as directional light sources.
            if (!is_envmap(light)) {
                dir_light = normalize(point_on_light.position - vertex.position);
                // If the point on light is occluded, G is 0. So we need to test for occlusion.
                // To avoid self intersection, we need to set the tnear of the ray
                // to a small "epsilon" which we define as c_shadow_epsilon as a global constant.
                Ray shadow_ray{vertex.position, dir_light, 
                               c_shadow_epsilon,
                               (1 - c_shadow_epsilon) * distance(point_on_light.position, vertex.position)};
                if (!occluded(scene, shadow_ray)) {
                    // geometry term is cosine at v_{i+1} divided by distance squared
                    // this can be derived by the infinitesimal area of a surface projected on
                    // a unit sphere -- it's the Jacobian between the area measure and the solid angle
                    // measure.
                    G = max(-dot(dir_light, point_on_light.normal), Real(0)) /
                        distance_squared(point_on_light.position, vertex.position);
                }
            } else {
                // The direction from envmap towards the point is stored in
                // point_on_light.normal.
                dir_light = -point_on_light.normal;
                // If the point on light is occluded, G is 0. So we need to test for occlusion.
                // To avoid self intersection, we need to set the tnear of the ray
                // to a small "epsilon" which we define as c_shadow_epsilon as a global constant.
                Ray shadow_ray{vertex.position, dir_light, 
                               c_shadow_epsilon,
                               infinity<Real>() /* envmaps are infinitely far away */};
                if (!occluded(scene, shadow_ray)) {
                    // We integrate envmaps using the solid angle measure,
                    // so the geometry term is 1.
                    G = 1;
                }
            }

            // Before we proceed, we first compute the probability density p1(v1)
            // The probability density for light sampling to sample our point is
            // just the probability of sampling a light times the probability of sampling a point
            Real p1 = light_pmf(scene, light_id) *
                pdf_point_on_light(light, point_on_light, scene);

            // We don't need to continue the computation if G is 0.
            // Also sometimes there can be some numerical issue such that we generate
            // a light path with probability zero
            if (G > 0 && p1 > 0) {
                // Let's compute f (BSDF) next.
                Spectrum f;
                Vector3 dir_view = -ray.dir;
                assert(vertex.material_id >= 0);
                f = eval(mat, dir_light, dir_view, vertex, ray_diff.radius, scene.texture_pool);
                // L is stored in LightSampleRecord
                Spectrum L = lr.radiance;

                // C1 is just a product of all of them!
                C1 = G * f * L;
            
                // Next let's compute w1

                // Remember that we want to set
                // w1 = p_1(v^1)^2 / (p_1(v^1)^2 + p_2(v^1)^2)
                // Notice that all of the probability density share the same path prefix and those cancel out.
                // Therefore we only need to account for the generation of the vertex v_{i+1}.

                // The probability density for our hemispherical sampling to sample 
                Real p2 = pdf_sample_bsdf(
                    mat, dir_light, dir_view, vertex, ray_diff.radius, scene.texture_pool);
                // !!!! IMPORTANT !!!!
                // In general, p1 and p2 now live in different spaces!!
                // our BSDF API outputs a probability density in the solid angle measure
                // while our light probability density is in the area measure.
                // We need to make sure that they are in the same space.
                // This can be done by accounting for the Jacobian of the transformation
                // between the two measures.
                // In general, I recommend to transform everything to area measure 
                // (except for directional lights) since it fits to the path-space math better.
                // Converting a solid angle measure to an area measure is just a
                // multiplication of the geometry term G (let solid angle be dS, area be dA,
                // we have dA/dS = G).
                p2 *= G;

                w1 = (p1*p1) / (p1*p1 + p2*p2);
                C1 /= p1;
            }
        }
        radiance += (current_path_contrib / current_path_pdf) * C1 * w1;

        // Let's do the hemispherical sampling next.
        Vector3 dir_view = -ray.dir;
        Vector2 bsdf_rnd_param{next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng)};
        std::optional<Vector3> dir_bsdf_ =
            sample_bsdf(mat, dir_view, vertex, ray_diff.radius, scene.texture_pool, bsdf_rnd_param);
        if (!dir_bsdf_) {
            // BSDF sampling failed. Abort the loop.
            break;
        }
        Vector3 dir_bsdf = *dir_bsdf_;
        // Trace a ray towards bsdf_dir. Note that again we have
        // to have an "epsilon" tnear to prevent self intersection.
        Ray bsdf_ray{vertex.position, dir_bsdf, c_isect_epsilon, infinity<Real>()};
        std::optional<PathVertex> bsdf_vertex = intersect(scene, bsdf_ray);

        // To update current_path_contrib & current_path_pdf,
        // we need to multiply G(v_{i}, v_{i+1}) * f(v_{i-1}, v_{i}, v_{i+1}) to current_path_contrib
        // and the pdf for getting v_{i+1} using hemisphere sampling.
        Real G;
        if (bsdf_vertex) {
            G = fabs(dot(dir_bsdf, bsdf_vertex->geometry_normal)) /
                distance_squared(bsdf_vertex->position, vertex.position);
        } else {
            // We hit nothing, set G to 1 to account for the environment map contribution.
            G = 1;
        }

        Spectrum f;
        f = eval(mat, dir_bsdf, dir_view, vertex, ray_diff.radius, scene.texture_pool);
        Real p2 = pdf_sample_bsdf(mat, dir_bsdf, dir_view, vertex, ray_diff.radius, scene.texture_pool);
        if (p2 <= 0) {
            // Numerical issue -- we generated some invalid rays.
            break;
        }

        // Remember to convert p2 to area measure!
        p2 *= G;
        // note that G cancels out in the division f/p, but we still need
        // G later for the calculation of w2.

        // Now we want to check whether dir_bsdf hit a light source, and
        // account for the light contribution (C2 & w2 & p2).
        // There are two possibilities: either we hit an emissive surface,
        // or we hit an environment map.
        // We will handle them separately.
        if (bsdf_vertex && is_light(scene.shapes[bsdf_vertex->shape_id])) {
            // G & f are already computed.
            RayDifferential next_ray_diff = transfer(ray_diff, distance(ray.org, bsdf_vertex->position));
            Spectrum L = emission(*bsdf_vertex, -dir_bsdf, next_ray_diff.radius, scene);
            Spectrum C2 = G * f * L;
            // Next let's compute p1(v2): the probability of the light source sampling
            // directly drawing the point corresponds to bsdf_dir.
            int light_id = get_area_light_id(scene.shapes[bsdf_vertex->shape_id]);
            assert(light_id >= 0);
            const Light &light = scene.lights[light_id];
            PointAndNormal light_point{bsdf_vertex->position, bsdf_vertex->geometry_normal};
            Real p1 = light_pmf(scene, light_id) * pdf_point_on_light(light, light_point, scene);
            Real w2 = (p2*p2) / (p1*p1 + p2*p2);

            C2 /= p2;
            radiance += (current_path_contrib / current_path_pdf) * C2 * w2;
        } else if (!bsdf_vertex && has_envmap(scene)) {
            // G & f are already computed.
            const Light &light = get_envmap(scene);
            Spectrum L = emission(light,
                                  -dir_bsdf, // pointing outwards from light
                                  ray_diff.spread,
                                  PointAndNormal{}, // dummy parameter for envmap
                                  scene);
            Spectrum C2 = G * f * L;
            // Next let's compute p1(v2): the probability of the light source sampling
            // directly drawing the direction bsdf_dir.
            PointAndNormal light_point{Vector3{0, 0, 0}, dir_bsdf}; // pointing towards the scene
            Real p1 = light_pmf(scene, scene.envmap_light_id) *
                      pdf_point_on_light(light, light_point, scene);
            Real w2 = (p2*p2) / (p1*p1 + p2*p2);

            C2 /= p2;
            radiance += (current_path_contrib / current_path_pdf) * C2 * w2;
        }

        if (!bsdf_vertex) {
            // Hit nothing -- can't continue tracing.
            break;
        }

        // Update rays/ray differentials/intersection/current_path_contrib/current_pdf
        Real roughness = get_roughness(mat, vertex, ray_diff.radius, scene.texture_pool);
        // TODO: add refraction.
        ray_diff = reflect(ray_diff, vertex.mean_curvature, roughness);
        ray_diff = transfer(ray_diff, distance(vertex.position, bsdf_vertex->position));
        ray = bsdf_ray;
        vertex = *bsdf_vertex;
        current_path_contrib = current_path_contrib * G * f;
        current_path_pdf = current_path_pdf * p2;
    }

    return radiance;
}

std::shared_ptr<Image3> path_render(const Scene &scene) {
    int w = scene.camera.width, h = scene.camera.height;
    std::shared_ptr<Image3> img_ = std::make_shared<Image3>(w, h);
    Image3 &img = *img_;
    pcg32_state rng = init_pcg32();
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Spectrum radiance = make_zero_spectrum();
            int spp = scene.options.samples_per_pixel;
            for (int s = 0; s < spp; s++) {
                radiance += path_tracing(scene, x, y, rng);
            }
            img(x, y) = radiance / Real(spp);
        }
    }
    return img_;
}

std::shared_ptr<Image3> render(const Scene &scene) {
    if (scene.options.integrator == Integrator::Depth ||
            scene.options.integrator == Integrator::MeanCurvature ||
            scene.options.integrator == Integrator::RayDifferential ||
            scene.options.integrator == Integrator::MipmapLevel) {
        return aux_render(scene);
    } else if (scene.options.integrator == Integrator::Path) {
        return path_render(scene);
    } else {
        assert(false);
        return std::shared_ptr<Image3>();
    }
}
