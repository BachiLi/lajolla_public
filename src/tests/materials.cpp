#include "../material.h"
#include "../microfacet.h"
#include <cstdio>

Real compute_determinant(const Material &m,
                         const PathVertex &vertex,
                         const Vector3 &dir_in,
                         const Vector2 &rnd_param,
                         Real w) {
    Real eps = Real(1e-6);
    std::optional<BSDFSampleRecord> sample =
        sample_bsdf(m,
                    dir_in,
                    vertex, 
                    TexturePool(),
                    rnd_param,
                    w);
    std::optional<BSDFSampleRecord> sample_u =
        sample_bsdf(m,
                    dir_in,
                    vertex, 
                    TexturePool(),
                    rnd_param + Vector2{eps, Real(0)},
                    w);
    std::optional<BSDFSampleRecord> sample_v =
        sample_bsdf(m,
                    dir_in,
                    vertex, 
                    TexturePool(),
                    rnd_param + Vector2{Real(0), eps},
                    w);
    if (!sample || !sample_u || !sample_v) {
        printf("FAIL\n");
        exit(1);
        return 0;
    }
    Vector3 dir_u = (sample_u->dir_out - sample->dir_out) / eps;
    Vector3 dir_v = (sample_v->dir_out - sample->dir_out) / eps;
    // We have a 3x2 matrix
    // [dx/du dy/du dz/du]
    // [dx/dv dy/dv dz/dv]
    // We will generate a Gram matrix and take the square root of the determinant
    // [            dx/du^2 + dy/du^2 + dz/du^2, dx/du*dx/dv + dy/du*dy/dv + dz/du*dz/dv]
    // [dx/du*dx/dv + dy/du*dy/dv + dz/du*dz/dv,             dx/dv^2 + dy/dv^2 + dz/dv^2]
    // The determinant is
    // (dx/du^2 + dy/du^2 + dz/du^2) * (dx/dv^2 + dy/dv^2 + dz/dv^2) -
    //   (dx/du*dx/dv + dy/du*dy/dv + dz/du*dz/dv) * (dx/du*dx/dv + dy/du*dy/dv + dz/du*dz/dv)
    Real det = (dir_u.x * dir_u.x + dir_u.y * dir_u.y + dir_u.z * dir_u.z) *
               (dir_v.x * dir_v.x + dir_v.y * dir_v.y + dir_v.z * dir_v.z) -
               (dir_u.x * dir_v.x + dir_u.y * dir_v.y + dir_u.z * dir_v.z) *
               (dir_u.x * dir_v.x + dir_u.y * dir_v.y + dir_u.z * dir_v.z);
    return sqrt(det);
}

int main(int argc, char *argv[]) {
    // We'll just make sure sampling & PDF are consistent
    Vector2 rnd_param_uv{Real(0.3), Real(0.4)};
    Real rnd_param_w{Real(0.6)};
    Vector3 dir_in = normalize(Vector3{0.3, 0.4, 0.5});
    PathVertex vertex;
    vertex.geometric_normal = Vector3{0, 0, 1};
    vertex.shading_frame = Frame(vertex.geometric_normal);
    {
        Material m = Lambertian{ConstantTexture<Spectrum>{
            Vector3{Real(0.5), Real(0.5), Real(0.5)}}};

        Real det = compute_determinant(m, vertex, dir_in, rnd_param_uv, rnd_param_w);
        std::optional<BSDFSampleRecord> sample =
            sample_bsdf(m,
                        dir_in,
                        vertex, 
                        TexturePool(),
                        rnd_param_uv,
                        rnd_param_w);
        Real pdf = pdf_sample_bsdf(m, dir_in, sample->dir_out, vertex, TexturePool());
        if (fabs((1/det) - pdf) / fabs(pdf) > Real(1e-2)) {
            printf("FAIL\n");
            return 1;
        }
    }

    {
        Material m = RoughPlastic{
            ConstantTexture<Spectrum>{Vector3{Real(0.5), Real(0.5), Real(0.5)}},
            ConstantTexture<Spectrum>{Vector3{Real(0.5), Real(0.5), Real(0.5)}},
            ConstantTexture<Real>{Real(0.3)},
            Real(1.5)};

        // Roughplastic has 2 layers: we want to make sure we hit both for PDF computation
        Real det0 = compute_determinant(m, vertex, dir_in, rnd_param_uv, Real(0));
        Real det1 = compute_determinant(m, vertex, dir_in, rnd_param_uv, Real(1));
        std::optional<BSDFSampleRecord> sample =
            sample_bsdf(m,
                        dir_in,
                        vertex, 
                        TexturePool(),
                        rnd_param_uv,
                        Real(0) /* w shouldn't matter here */);
        Real pdf = pdf_sample_bsdf(m, dir_in, sample->dir_out, vertex, TexturePool());
        if (fabs(((1/det0) + (1/det1))/2 - pdf) / fabs(pdf) > Real(1e-2)) {
            printf("FAIL\n");
            return 1;
        }
    }

    {
        Material m = RoughDielectric{
            ConstantTexture<Spectrum>{Vector3{Real(0.5), Real(0.5), Real(0.5)}},
            ConstantTexture<Spectrum>{Vector3{Real(0.5), Real(0.5), Real(0.5)}},
            ConstantTexture<Real>{Real(0.3)},
            Real(1.5)};

        // Dielectric can refract or reflect. Make sure we test both.
        {
            Real det0 = compute_determinant(m, vertex, dir_in, rnd_param_uv, Real(0));
            std::optional<BSDFSampleRecord> sample_0 =
                sample_bsdf(m,
                            dir_in,
                            vertex, 
                            TexturePool(),
                            rnd_param_uv,
                            Real(0));
            Real pdf0 = pdf_sample_bsdf(m, dir_in, sample_0->dir_out, vertex, TexturePool());
            // Unfortunately we need to manually add the Fresnel term
            bool reflect = dot(vertex.geometric_normal, dir_in) *
                           dot(vertex.geometric_normal, sample_0->dir_out) > 0;
            Vector3 half_vector;
            if (reflect) {
                half_vector = normalize(dir_in + sample_0->dir_out);
            } else {
                half_vector = normalize(dir_in + sample_0->dir_out * Real(1.5));
            }
            Real h_dot_in = dot(half_vector, dir_in);
            Real F = fresnel_dielectric(h_dot_in, Real(1.5));
            Real inv_det0 = Real(1) / det0;
            if (reflect) {
                inv_det0 *= F;
            } else {
                inv_det0 *= (1 - F);
            }
            
            if (fabs(inv_det0 - pdf0) / fabs(pdf0) > Real(1e-2)) {
                printf("FAIL\n");
                return 1;
            }
        }

        {
            Real det1 = compute_determinant(m, vertex, dir_in, rnd_param_uv, Real(1));
            std::optional<BSDFSampleRecord> sample_1 =
                sample_bsdf(m,
                            dir_in,
                            vertex, 
                            TexturePool(),
                            rnd_param_uv,
                            Real(1));
            Real pdf1 = pdf_sample_bsdf(m, dir_in, sample_1->dir_out, vertex, TexturePool());
            // Unfortunately we need to manually add the Fresnel term
            bool reflect = dot(vertex.geometric_normal, dir_in) *
                           dot(vertex.geometric_normal, sample_1->dir_out) > 0;
            Vector3 half_vector;
            if (reflect) {
                half_vector = normalize(dir_in + sample_1->dir_out);
            } else {
                half_vector = normalize(dir_in + sample_1->dir_out * Real(1.5));
            }
            Real h_dot_in = dot(half_vector, dir_in);
            Real F = fresnel_dielectric(h_dot_in, Real(1.5));
            Real inv_det1 = Real(1) / det1;
            if (reflect) {
                inv_det1 *= F;
            } else {
                inv_det1 *= (1 - F);
            }
            
            if (fabs(inv_det1 - pdf1) / fabs(pdf1) > Real(1e-2)) {
                printf("FAIL\n");
                return 1;
            }
        }
    }

    printf("SUCCESS\n");
    return 0;
}
