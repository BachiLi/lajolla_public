#pragma once

#include "lajolla.h"
#include "spectrum.h"
#include "texture.h"
#include "vector.h"
#include <optional>
#include <variant>

struct PathVertex;

struct Lambertian {
    Texture<Spectrum> reflectance;
};

/// The RoughPlastic material is a 2-layer BRDF with a dielectric coating
/// and a diffuse layer. The dielectric coating uses a GGX microfacet model
/// and the diffuse layer is Lambertian.
/// Unlike Mitsuba, we ignore the internal scattering between the
/// dielectric layer and the diffuse layer: theoretically, a ray can
/// bounce between the two layers inside the material. This is expensive
/// to simulate and also creates a complex relation between the reflectances
/// and the color. Mitsuba resolves the computational cost using a precomputed table,
/// but that makes the whole renderer architecture too complex.
struct RoughPlastic {
    Texture<Spectrum> diffuse_reflectance;
    Texture<Spectrum> specular_reflectance;
    Texture<Real> roughness;

    Real eta; // internal IOR / externalIOR
};

// To add more materials, first create a struct for the material, then overload the () operators for all the
// functors below.
using Material = std::variant<Lambertian, RoughPlastic>;

/// Given incoming direction and outgoing direction of lights,
/// both pointing outwards of the surface point,
/// outputs the BSDF times the cosine between outgoing direction
/// and the shading normal, evaluated at a point.
Spectrum eval(const Material &material,
              const Vector3 &dir_light,
              const Vector3 &dir_view,
              const PathVertex &vertex,
              Real ray_footprint, // for texture filtering
              const TexturePool &texture_pool);

/// We allow non-reciprocal BRDFs, so it's important
/// to distinguish which direction we are tracing the rays.
enum class TransportDirection {
    TO_LIGHT,
    TO_VIEW
};

/// When sampling a material (BSDF), in addition to returning
/// the outgoing direction in the world coordinates, we also
/// provide the evaluation of the BSDF and the probability density
/// of the sampling procedure.
/// It is of course possible to evaluate eval & pdf using eval() and
/// pdf_sample_bsdf() later. However, it is crucial for
/// numerical accuracy, especially for very low roughness materials,
/// that we evaluate the BSDF/PDF while sampling the materials, otherwise
/// we suffer from floating point number precision loss when transforming/
/// normalizing vectors, etc.
struct MaterialSampleRecord {
    Vector3 dir;
    Spectrum eval;
    Real pdf;
};

/// Given incoming direction pointing outwards of the surface point,
/// samples an outgoing direction.
/// If dir == TO_LIGHT, incoming direction is dir_view and 
/// we're sampling for dir_light. Vice versa.
std::optional<Vector3> sample_bsdf(const Material &material,
                                   const Vector3 &dir_in,
                                   const PathVertex &vertex,
                                   Real ray_footprint, // for texture filtering
                                   const TexturePool &texture_pool,
                                   const Vector2 &rnd_param,
                                   TransportDirection dir = TransportDirection::TO_LIGHT);

/// Given incoming direction and outgoing direction of lights,
/// both pointing outwards of the surface point,
/// outputs the probability density of sampling.
/// If dir == TO_LIGHT, incoming direction is dir_view and 
/// we're sampling for dir_light. Vice versa.
Real pdf_sample_bsdf(const Material &material,
                     const Vector3 &dir_light,
                     const Vector3 &dir_view,
                     const PathVertex &vertex,
                     Real ray_footprint, // for texture filtering
                     const TexturePool &texture_pool,
                     TransportDirection dir = TransportDirection::TO_LIGHT);

Real get_roughness(const Material &material,
                   const PathVertex &vertex,
                   Real ray_footprint,
                   const TexturePool &texture_pool);
const TextureSpectrum &get_texture(const Material &material);
