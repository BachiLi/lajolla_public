#pragma once

#include "lajolla.h"
#include "image.h"
#include "intersection.h"
#include "mipmap.h"
#include <map>
#include <variant>

/// Can be replaced by a more advanced texture caching system,
/// where we only load images from files when necessary.
/// See OpenImageIO for example https://github.com/OpenImageIO/oiio
struct TexturePool {
    std::map<std::string, int> image1s_map;
    std::map<std::string, int> image3s_map;

    std::vector<Mipmap1> image1s;
    std::vector<Mipmap3> image3s;
};

inline int insert_image1(TexturePool &pool, const std::string &texture_name, const fs::path &filename) {
    if (pool.image1s_map.find(texture_name) != pool.image1s_map.end()) {
        // We don't check if img is the same as the one in the cache!
        return pool.image1s_map[texture_name];
    }
    int id = (int)pool.image1s.size();
    pool.image1s_map[texture_name] = id;
    pool.image1s.push_back(make_mipmap(imread1(filename)));
    return id;
}

inline int insert_image1(TexturePool &pool, const std::string &texture_name, const Image1 &img) {
    if (pool.image1s_map.find(texture_name) != pool.image1s_map.end()) {
        // We don't check if img is the same as the one in the cache!
        return pool.image1s_map[texture_name];
    }
    int id = (int)pool.image1s.size();
    pool.image1s_map[texture_name] = id;
    pool.image1s.push_back(make_mipmap(img));
    return id;
}

inline int insert_image3(TexturePool &pool, const std::string &texture_name, const fs::path &filename) {
    if (pool.image3s_map.find(texture_name) != pool.image3s_map.end()) {
        // We don't check if img is the same as the one in the cache!
        return pool.image3s_map[texture_name];
    }
    int id = (int)pool.image3s.size();
    pool.image3s_map[texture_name] = id;
    pool.image3s.push_back(make_mipmap(imread3(filename)));
    return id;
}

inline int insert_image3(TexturePool &pool, const std::string &texture_name, const Image3 &img) {
    if (pool.image3s_map.find(texture_name) != pool.image3s_map.end()) {
        // We don't check if img is the same as the one in the cache!
        return pool.image3s_map[texture_name];
    }
    int id = (int)pool.image3s.size();
    pool.image3s_map[texture_name] = id;
    pool.image3s.push_back(make_mipmap(img));
    return id;
}

inline const Mipmap1 &get_img1(const TexturePool &pool, int texture_id) {
    assert(texture_id >= 0 && texture_id < pool.image1s.size());
    return pool.image1s[texture_id];
}

inline const Mipmap3 &get_img3(const TexturePool &pool, int texture_id) {
    assert(texture_id >= 0 && texture_id < pool.image3s.size());
    return pool.image3s[texture_id];
}

template <typename T>
struct ConstantTexture {
    T value;
};

template <typename T>
struct ImageTexture {
    int texture_id;
    Real uscale, vscale;
};

template <typename T>
inline const Mipmap<T> &get_img(const ImageTexture<T> &t, const TexturePool &pool) {
    return Mipmap<T>{};
}
template <>
inline const Mipmap<Real> &get_img(const ImageTexture<Real> &t, const TexturePool &pool) {
    return get_img1(pool, t.texture_id);
}
template <>
inline const Mipmap<Vector3> &get_img(const ImageTexture<Vector3> &t, const TexturePool &pool) {
    return get_img3(pool, t.texture_id);
}

template <typename T>
using Texture = std::variant<ConstantTexture<T>, ImageTexture<T>>;
using Texture1 = Texture<Real>;
using TextureSpectrum = Texture<Spectrum>;

template <typename T>
struct eval_texture_op {
    T operator()(const ConstantTexture<T> &t) const;
    T operator()(const ImageTexture<T> &t) const;

    const PathVertex &vertex;
    const Real &footprint;
    const TexturePool &pool;
};
template <typename T>
T eval_texture_op<T>::operator()(const ConstantTexture<T> &t) const {
    return t.value;
}
template <typename T>
T eval_texture_op<T>::operator()(const ImageTexture<T> &t) const {
    const Mipmap<T> &img = get_img(t, pool);
    Vector2 uv{modulo(vertex.uv[0] * t.uscale, Real(1)),
               modulo(vertex.uv[1] * t.vscale, Real(1))};
    Real scaled_footprint = max(get_width(img), get_height(img)) * max(t.uscale, t.vscale) * footprint;
    Real level = log2(max(footprint, Real(1e-8f)));
    return lookup(img, uv[0], uv[1], level);
}

template <typename T>
T eval(const Texture<T> &texture, const PathVertex &vertex, Real footprint, const TexturePool &pool) {
    return std::visit(eval_texture_op<T>{vertex, footprint, pool}, texture);
}

inline ConstantTexture<Spectrum> make_constant_spectrum_texture(const Spectrum &spec) {
    return ConstantTexture<Spectrum>{spec};
}

inline ConstantTexture<Real> make_constant_float_texture(Real f) {
    return ConstantTexture<Real>{f};
}

inline ImageTexture<Spectrum> make_image_spectrum_texture(
        const std::string &texture_name,
        const fs::path &filename,
        TexturePool &pool,
        Real uscale = 1,
        Real vscale = 1) {
    return ImageTexture<Spectrum>{insert_image3(pool, texture_name, filename), uscale, vscale};
}

inline ImageTexture<Spectrum> make_image_spectrum_texture(
        const std::string &texture_name,
        const Image3 &img,
        TexturePool &pool,
        Real uscale = 1,
        Real vscale = 1) {
    return ImageTexture<Spectrum>{insert_image3(pool, texture_name, img), uscale, vscale};
}

inline ImageTexture<Real> make_image_float_texture(
        const std::string &texture_name,
        const fs::path &filename,
        TexturePool &pool,
        Real uscale = 1,
        Real vscale = 1) {
    return ImageTexture<Real>{insert_image1(pool, texture_name, filename), uscale, vscale};
}

inline ImageTexture<Real> make_image_float_texture(
        const std::string &texture_name,
        const Image1 &img,
        TexturePool &pool,
        Real uscale = 1,
        Real vscale = 1) {
    return ImageTexture<Real>{insert_image1(pool, texture_name, img), uscale, vscale};
}
