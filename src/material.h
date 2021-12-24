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

    // Note that the material is not transmissive.
    // This is for the IOR between the dielectric layer and the diffuse layer.
    Real eta; // internal IOR / externalIOR
};

/// The roughdielectric BSDF implements a version of Walter et al.'s
/// "Microfacet Models for Refraction through Rough Surfaces"
/// The key idea is to define a microfacet model where the normals
/// are centered at a "generalized half-vector" wi + wo * eta.
/// This gives a glossy/specular transmissive BSDF.
struct RoughDielectric {
    Texture<Spectrum> specular_reflectance;
    Texture<Spectrum> specular_transmittance;
    Texture<Real> roughness;

    Real eta; // internal IOR / externalIOR
};

/// For homework 1: the diffuse and subsurface component of the Disney BRDF.
struct DisneyDiffuse {
    Texture<Spectrum> base_color;
    Texture<Real> roughness;
    Texture<Real> subsurface;
};

/// For homework 1: the metallic component of the Disney BRDF.
struct DisneyMetal {
    Texture<Spectrum> base_color;
    Texture<Real> roughness;
    Texture<Real> anisotropic;
};

/// For homework 1: the transmissive component of the Disney BRDF.
struct DisneyGlass {
    Texture<Spectrum> base_color;
    Texture<Real> roughness;
    Texture<Real> anisotropic;

    Real eta; // internal IOR / externalIOR
};

/// For homework 1: the clearcoat component of the Disney BRDF.
struct DisneyClearcoat {
    Texture<Real> clearcoat_gloss;
};

/// For homework 1: the sheen component of the Disney BRDF.
struct DisneySheen {
    Texture<Spectrum> base_color;
    Texture<Real> sheen_tint;
};

/// For homework 1: the whole Disney BRDF.
struct DisneyBSDF {
    Texture<Spectrum> base_color;
    Texture<Real> specular_transmission;
    Texture<Real> metallic;
    Texture<Real> subsurface;
    Texture<Real> specular;
    Texture<Real> roughness;
    Texture<Real> specular_tint;
    Texture<Real> anisotropic;
    Texture<Real> sheen;
    Texture<Real> sheen_tint;
    Texture<Real> clearcoat;
    Texture<Real> clearcoat_gloss;

    Real eta;
};

// To add more materials, first create a struct for the material, then overload the () operators for all the
// functors below.
using Material = std::variant<Lambertian,
                              RoughPlastic,
                              RoughDielectric,
                              DisneyDiffuse,
                              DisneyMetal,
                              DisneyGlass,
                              DisneyClearcoat,
                              DisneySheen,
                              DisneyBSDF>;

/// We allow non-reciprocal BRDFs, so it's important
/// to distinguish which direction we are tracing the rays.
enum class TransportDirection {
    TO_LIGHT,
    TO_VIEW
};

/// Given incoming direction and outgoing direction of lights,
/// both pointing outwards of the surface point,
/// outputs the BSDF times the cosine between outgoing direction
/// and the shading normal, evaluated at a point.
/// When the transport direction is towards the lights,
/// dir_in is the view direction, and dir_out is the light direction.
/// Vice versa.
Spectrum eval(const Material &material,
              const Vector3 &dir_in,
              const Vector3 &dir_out,
              const PathVertex &vertex,
              const TexturePool &texture_pool,
              TransportDirection dir = TransportDirection::TO_LIGHT);

struct BSDFSampleRecord {
    Vector3 dir_out;
    // The index of refraction ratio. Set to 0 if it's not a transmission event.
    Real eta;
    Real roughness; // Roughness of the selected BRDF layer ([0, 1]).
};

/// Given incoming direction pointing outwards of the surface point,
/// samples an outgoing direction. Also returns the index of refraction
/// and the roughness of the selected BSDF layer for path tracer's use.
/// Return an invalid value if the sampling
/// failed (e.g., if the incoming direction is invalid).
/// If dir == TO_LIGHT, incoming direction is the view direction and 
/// we're sampling for the light direction. Vice versa.
std::optional<BSDFSampleRecord> sample_bsdf(
    const Material &material,
    const Vector3 &dir_in,
    const PathVertex &vertex,
    const TexturePool &texture_pool,
    const Vector2 &rnd_param_uv,
    const Real &rnd_param_w,
    TransportDirection dir = TransportDirection::TO_LIGHT);

/// Given incoming direction and outgoing direction of lights,
/// both pointing outwards of the surface point,
/// outputs the probability density of sampling.
/// If dir == TO_LIGHT, incoming direction is dir_view and 
/// we're sampling for dir_light. Vice versa.
Real pdf_sample_bsdf(const Material &material,
                     const Vector3 &dir_in,
                     const Vector3 &dir_out,
                     const PathVertex &vertex,
                     const TexturePool &texture_pool,
                     TransportDirection dir = TransportDirection::TO_LIGHT);

/// Return a texture from the material for debugging.
/// If the material contains multiple textures, return an arbitrary one.
TextureSpectrum get_texture(const Material &material);
