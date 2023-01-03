#include "material.h"
#include "intersection.h"

inline Vector3 sample_cos_hemisphere(const Vector2 &rnd_param) {
    Real phi = c_TWOPI * rnd_param[0];
    Real tmp = sqrt(std::clamp(1 - rnd_param[1], Real(0), Real(1)));
    return Vector3{
        cos(phi) * tmp, sin(phi) * tmp,
        sqrt(std::clamp(rnd_param[1], Real(0), Real(1)))
    };
}

struct eval_op {
    Spectrum operator()(const Lambertian &bsdf) const;
    Spectrum operator()(const RoughPlastic &bsdf) const;
    Spectrum operator()(const RoughDielectric &bsdf) const;
    Spectrum operator()(const DisneyDiffuse &bsdf) const;
    Spectrum operator()(const DisneyMetal &bsdf) const;
    Spectrum operator()(const DisneyGlass &bsdf) const;
    Spectrum operator()(const DisneyClearcoat &bsdf) const;
    Spectrum operator()(const DisneySheen &bsdf) const;
    Spectrum operator()(const DisneyBSDF &bsdf) const;

    const Vector3 &dir_in;
    const Vector3 &dir_out;
    const PathVertex &vertex;
    const TexturePool &texture_pool;
    const TransportDirection &dir;
};

struct pdf_sample_bsdf_op {
    Real operator()(const Lambertian &bsdf) const;
    Real operator()(const RoughPlastic &bsdf) const;
    Real operator()(const RoughDielectric &bsdf) const;
    Real operator()(const DisneyDiffuse &bsdf) const;
    Real operator()(const DisneyMetal &bsdf) const;
    Real operator()(const DisneyGlass &bsdf) const;
    Real operator()(const DisneyClearcoat &bsdf) const;
    Real operator()(const DisneySheen &bsdf) const;
    Real operator()(const DisneyBSDF &bsdf) const;

    const Vector3 &dir_in;
    const Vector3 &dir_out;
    const PathVertex &vertex;
    const TexturePool &texture_pool;
    const TransportDirection &dir;
};

struct sample_bsdf_op {
    std::optional<BSDFSampleRecord> operator()(const Lambertian &bsdf) const;
    std::optional<BSDFSampleRecord> operator()(const RoughPlastic &bsdf) const;
    std::optional<BSDFSampleRecord> operator()(const RoughDielectric &bsdf) const;
    std::optional<BSDFSampleRecord> operator()(const DisneyDiffuse &bsdf) const;
    std::optional<BSDFSampleRecord> operator()(const DisneyMetal &bsdf) const;
    std::optional<BSDFSampleRecord> operator()(const DisneyGlass &bsdf) const;
    std::optional<BSDFSampleRecord> operator()(const DisneyClearcoat &bsdf) const;
    std::optional<BSDFSampleRecord> operator()(const DisneySheen &bsdf) const;
    std::optional<BSDFSampleRecord> operator()(const DisneyBSDF &bsdf) const;

    const Vector3 &dir_in;
    const PathVertex &vertex;
    const TexturePool &texture_pool;
    const Vector2 &rnd_param_uv;
    const Real &rnd_param_w;
    const TransportDirection &dir;
};

struct get_texture_op {
    TextureSpectrum operator()(const Lambertian &bsdf) const;
    TextureSpectrum operator()(const RoughPlastic &bsdf) const;
    TextureSpectrum operator()(const RoughDielectric &bsdf) const;
    TextureSpectrum operator()(const DisneyDiffuse &bsdf) const;
    TextureSpectrum operator()(const DisneyMetal &bsdf) const;
    TextureSpectrum operator()(const DisneyGlass &bsdf) const;
    TextureSpectrum operator()(const DisneyClearcoat &bsdf) const;
    TextureSpectrum operator()(const DisneySheen &bsdf) const;
    TextureSpectrum operator()(const DisneyBSDF &bsdf) const;
};

#include "materials/lambertian.inl"
#include "materials/roughplastic.inl"
#include "materials/roughdielectric.inl"
#include "materials/disney_diffuse.inl"
#include "materials/disney_metal.inl"
#include "materials/disney_glass.inl"
#include "materials/disney_clearcoat.inl"
#include "materials/disney_sheen.inl"
#include "materials/disney_bsdf.inl"

Spectrum eval(const Material &material,
              const Vector3 &dir_in,
              const Vector3 &dir_out,
              const PathVertex &vertex,
              const TexturePool &texture_pool,
              TransportDirection dir) {
    return std::visit(eval_op{dir_in, dir_out, vertex, texture_pool, dir}, material);
}

std::optional<BSDFSampleRecord>
sample_bsdf(const Material &material,
            const Vector3 &dir_in,
            const PathVertex &vertex,
            const TexturePool &texture_pool,
            const Vector2 &rnd_param_uv,
            const Real &rnd_param_w,
            TransportDirection dir) {
    return std::visit(sample_bsdf_op{
        dir_in, vertex, texture_pool, rnd_param_uv, rnd_param_w, dir}, material);
}

Real pdf_sample_bsdf(const Material &material,
                     const Vector3 &dir_in,
                     const Vector3 &dir_out,
                     const PathVertex &vertex,
                     const TexturePool &texture_pool,
                     TransportDirection dir) {
    return std::visit(pdf_sample_bsdf_op{
        dir_in, dir_out, vertex, texture_pool, dir}, material);
}

TextureSpectrum get_texture(const Material &material) {
    return std::visit(get_texture_op{}, material);
}
