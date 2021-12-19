#include "light.h"
#include "scene.h"
#include "spectrum.h"
#include "transform.h"

////////////////////////////////////////////////////////////////////////////////
struct light_power_op {
    Real operator()(const DiffuseAreaLight &light) const;
    Real operator()(const Envmap &light) const;

    const Scene &scene;
};
Real light_power_op::operator()(const DiffuseAreaLight &light) const {
    return luminance(light.intensity) * surface_area(scene.shapes[light.shape_id]) * c_PI;
}
Real light_power_op::operator()(const Envmap &light) const {
    return c_PI * scene.bounds.radius * scene.bounds.radius *
           light.sampling_dist.total_values /
           (light.sampling_dist.width * light.sampling_dist.height);
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
struct sample_point_on_light_op {
    PointAndNormal operator()(const DiffuseAreaLight &light) const;
    PointAndNormal operator()(const Envmap &light) const;

    const Vector2 &rnd_param_uv;
    const Real &rnd_param_w;
    const Scene &scene;
};
PointAndNormal sample_point_on_light_op::operator()(const DiffuseAreaLight &light) const {
    const Shape &shape = scene.shapes[light.shape_id];
    return sample_point_on_shape(shape, rnd_param_uv, rnd_param_w);
}
PointAndNormal sample_point_on_light_op::operator()(const Envmap &light) const {
    Vector2 uv = sample(light.sampling_dist, rnd_param_uv);
    // Convert uv to spherical coordinates
    Real azimuth = uv[0] * (2 * c_PI);
    Real elevation = uv[1] * c_PI;
    // Convert spherical coordinates to Cartesian coordinates.
    // (https://en.wikipedia.org/wiki/Spherical_coordinate_system#Cartesian_coordinates)
    // We use the convention that y is the up axis.
    Vector3 local_dir{sin(azimuth) * sin(elevation),
                      cos(elevation),
                      -cos(azimuth) * sin(elevation)};
    Vector3 world_dir = xform_vector(light.to_world, local_dir);
    return PointAndNormal{Vector3{0, 0, 0}, -world_dir};
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
struct pdf_point_on_light_op {
    Real operator()(const DiffuseAreaLight &light) const;
    Real operator()(const Envmap &light) const;

    const PointAndNormal &point_on_light;
    const Scene &scene;
};
Real pdf_point_on_light_op::operator()(const DiffuseAreaLight &light) const {
    return pdf_point_on_shape(scene.shapes[light.shape_id]);
}
Real pdf_point_on_light_op::operator()(const Envmap &light) const {
    // We store the direction from light in point_on_light.normal.
    Vector3 world_dir = -point_on_light.normal;
    // Convert the direction to local Catesian coordinates.
    Vector3 local_dir = xform_vector(light.to_local, world_dir);
    // Convert the Cartesian coordinates to the spherical coordinates.
    // We use the convention that y is the up-axis.
    Vector2 uv{atan2(local_dir[0], -local_dir[2]) * c_INVTWOPI,
               acos(std::clamp(local_dir[1], Real(0), Real(1))) * c_INVPI};
    // atan2 returns -pi to pi, we map [-pi, 0] to [pi, 2pi]
    if (uv[0] < 0) {
        uv[0] += 1;
    }
    Real cos_elevation = local_dir.y;
    Real sin_elevation = sqrt(std::clamp(1 - cos_elevation * cos_elevation, Real(0), Real(1)));
    if (sin_elevation <= 0) {
        // degenerate
        return 0;
    }
    return pdf(light.sampling_dist, uv) / (2 * c_PI * c_PI * sin_elevation);
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
struct emission_op {
    Spectrum operator()(const DiffuseAreaLight &light) const;
    Spectrum operator()(const Envmap &light) const;

    const Vector3 &view_dir;
    const PointAndNormal &point_on_light;
    Real view_footprint;
    const Scene &scene;
};
Spectrum emission_op::operator()(const DiffuseAreaLight &light) const {
    if (dot(point_on_light.normal, view_dir) <= 0) {
        return make_zero_spectrum();
    }
    return light.intensity;
}
Spectrum emission_op::operator()(const Envmap &light) const {
    // View dir is pointing outwards "from" the light.
    // An environment map stores the light from the opposite direction,
    // so we need to flip view dir.
    // We then transform the direction to the local Cartesian coordinates.
    Vector3 local_dir = xform_vector(light.to_local, -view_dir);
    // Convert the Cartesian coordinates to the spherical coordinates.
    Vector2 uv{atan2(local_dir[0], -local_dir[2]) * c_INVTWOPI,
               acos(std::clamp(local_dir[1], Real(0), Real(1))) * c_INVPI};
    // atan2 returns -pi to pi, we map [-pi, 0] to [pi, 2pi]
    if (uv[0] < 0) {
        uv[0] += 1;
    }

    // For envmap, view_footprint stores (approximatedly) d view_dir / dx
    // We want to convert it to du/dx -- we do it by computing (d dir / dx) * (d u / d dir)
    // To do this we differentiate through the process above:
    
    // We abbrevite local_dir as w
    Vector3 w = local_dir;
    Real dudwx = -w.z / (w.x * w.x + w.z * w.z);
    Real dudwz = w.x / (w.x * w.x + w.z * w.z);
    Real dvdwy = -1 / sqrt(std::max(1 - w.y * w.y, Real(0)));
    // We only want to know the length of dudw & dvdw
    // The local coordinate transformation is length preserving,
    // so we don't need to differentiate through it.
    Real footprint = min(sqrt(dudwx * dudwx + dudwz * dudwz), dvdwy);
    // (I haven't see this in any renderer I know...the closest one is 
    // "real-time shading with filtered importance sampling" from Colbert et al.
    // I don't know why people don't do this.)

    return eval(light.values, uv, footprint, scene.texture_pool);
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
struct init_sampling_dist_op {
    void operator()(DiffuseAreaLight &light) const;
    void operator()(Envmap &light) const;

    const Scene &scene;
};
void init_sampling_dist_op::operator()(DiffuseAreaLight &light) const {
}
void init_sampling_dist_op::operator()(Envmap &light) const {
    if (auto *t = std::get_if<ImageTexture<Spectrum>>(&light.values)) {
        // Only need to initialize sampling distribution
        // if the envmap is an image.
        const Mipmap3 &mipmap = get_img(*t, scene.texture_pool);
        int w = get_width(mipmap), h = get_height(mipmap);
        std::vector<Real> f(w * h);
        int i = 0;
        for (int y = 0; y < h; y++) {
            // We shift the grids by 0.5 pixels because we are approximating
            // a piecewise bilinear distribution with a piecewise constant
            // distribution. This shifting is necessary to make the sampling
            // unbiased, as we can interpolate at a position of a black pixel
            // and get a non-zero contribution.
            Real v = (y + Real(0.5)) / Real(h);
            Real sin_elevation = sin(c_PI * v);
            for (int x = 0; x < w; x++) {
                Real u = (x + Real(0.5)) / Real(w);
                f[i++] = luminance(lookup(mipmap, u, v, 0)) * sin_elevation;
            }
        }
        light.sampling_dist = make_table_dist_2d(f, w, h);
    }
}
////////////////////////////////////////////////////////////////////////////////

Real light_power(const Light &light, const Scene &scene) {
    return std::visit(light_power_op{scene}, light);
}

PointAndNormal sample_point_on_light(const Light &light,
                                     const Vector2 &rnd_param_uv,
                                     Real rnd_param_w,
                                     const Scene &scene) {
    return std::visit(sample_point_on_light_op{rnd_param_uv, rnd_param_w, scene}, light);
}

Real pdf_point_on_light(const Light &light, const PointAndNormal &point_on_light, const Scene &scene) {
    return std::visit(pdf_point_on_light_op{point_on_light, scene}, light);
}

Spectrum emission(const Light &light,
                  const Vector3 &view_dir,
                  Real view_footprint,
                  const PointAndNormal &point_on_light,
                  const Scene &scene) {
    return std::visit(emission_op{view_dir, point_on_light, view_footprint, scene}, light);
}

void init_sampling_dist(Light &light, const Scene &scene) {
    return std::visit(init_sampling_dist_op{scene}, light);
}
