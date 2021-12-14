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

////////////////////////////////////////////////////////////////////////
struct eval_op {
    Spectrum operator()(const Lambertian &lambertian) const;

    const Vector3 &dir_light;
    const Vector3 &dir_view;
    const PathVertex &vertex;
    const TexturePool &texture_pool;
};
Spectrum eval_op::operator()(const Lambertian &lambertian) const {
    if (dot(dir_view, vertex.shading_frame.n) < 0) {
        // Incoming direction is below the surface.
        return make_zero_spectrum();
    }
	return fmax(dot(dir_light, vertex.shading_frame.n), Real(0)) * 
           eval(lambertian.reflectance, vertex, texture_pool) / c_PI;
}
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
struct pdf_sample_bsdf_op {
    Real operator()(const Lambertian &lambertian) const;

    const Vector3 &dir_light;
    const Vector3 &dir_view;
    const PathVertex &vertex;
    const TransportDirection &dir;
};
Real pdf_sample_bsdf_op::operator()(const Lambertian &lambertian) const {
    // For Lambertian, we importance sample the cosine hemisphere domain.
    if (dir == TransportDirection::TO_LIGHT) {
        if (dot(dir_view, vertex.shading_frame.n) < 0) {
            // Incoming direction is below the surface.
            return 0;
        }
        return fmax(dot(dir_light, vertex.shading_frame.n), Real(0)) / c_PI;
    } else {
        if (dot(dir_view, vertex.shading_frame.n) < 0) {
            // Incoming direction is below the surface.
            return 0;
        }
        return fmax(dot(dir_view, vertex.shading_frame.n), Real(0)) / c_PI;
    }
}
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
struct sample_bsdf_op {
    std::optional<Vector3> operator()(const Lambertian &lambertian) const;

    const Vector3 &dir_in;
    const PathVertex &vertex;
    const Vector2 &rnd_param;
    const TransportDirection &dir;
};
std::optional<Vector3> sample_bsdf_op::operator()(const Lambertian &lambertian) const {
    // For Lambertian, we importance sample the cosine hemisphere domain.
    if (dot(vertex.shading_frame.n, dir_in) < 0) {
        // Incoming direction is below the surface.
        return {};
    }
    return to_world(vertex.shading_frame, sample_cos_hemisphere(rnd_param));
}
////////////////////////////////////////////////////////////////////////

Spectrum eval(const Material &material,
              const Vector3 &dir_light,
              const Vector3 &dir_view,
              const PathVertex &vertex,
              const TexturePool &texture_pool) {
	return std::visit(eval_op{dir_light, dir_view, vertex, texture_pool}, material);
}

std::optional<Vector3> sample_bsdf(const Material &material,
                                   const Vector3 &dir_in,
                                   const PathVertex &vertex,
                                   const Vector2 &rnd_param,
                                   TransportDirection dir) {
    return std::visit(sample_bsdf_op{dir_in, vertex, rnd_param, dir}, material);
}

Real pdf_sample_bsdf(const Material &material,
                     const Vector3 &dir_light,
                     const Vector3 &dir_view,
                     const PathVertex &vertex,
                     TransportDirection dir) {
    return std::visit(pdf_sample_bsdf_op{dir_light, dir_view, vertex, dir}, material);
}
