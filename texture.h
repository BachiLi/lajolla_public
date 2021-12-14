#pragma once

#include "lajolla.h"
#include "image.h"
#include "intersection.h"
#include <map>
#include <variant>

/// Can be replaced by a more advanced texture caching system,
/// where we only load images from files when necessary.
/// See OpenImageIO for example https://github.com/OpenImageIO/oiio
struct TexturePool {
    std::map<std::string, int> image1s_map;
    std::map<std::string, int> image3s_map;

    std::vector<Image1> image1s;
    std::vector<Image3> image3s;
};

inline int insert_image1(TexturePool &pool, const std::string &texture_name, const fs::path &filename) {
    if (pool.image1s_map.find(texture_name) != pool.image1s_map.end()) {
        // We don't check if img is the same as the one in the cache!
        return pool.image1s_map[texture_name];
    }
    int id = (int)pool.image1s.size();
    pool.image1s_map[texture_name] = id;
    pool.image1s.push_back(imread1(filename));
    return id;
}

inline int insert_image3(TexturePool &pool, const std::string &texture_name, const fs::path &filename) {
    if (pool.image3s_map.find(texture_name) != pool.image3s_map.end()) {
        // We don't check if img is the same as the one in the cache!
        return pool.image3s_map[texture_name];
    }
    int id = (int)pool.image3s.size();
    pool.image3s_map[texture_name] = id;
    pool.image3s.push_back(imread3(filename));
    return id;
}

inline const Image1 &get_img1(const TexturePool &pool, int texture_id) {
    return pool.image1s[texture_id];
}

inline const Image3 &get_img3(const TexturePool &pool, int texture_id) {
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
inline const Image<T> &get_img(const ImageTexture<T> &t, const TexturePool &pool) {
    return Image<T>{};
}
template <>
inline const Image<Real> &get_img(const ImageTexture<Real> &t, const TexturePool &pool) {
    return get_img1(pool, t.texture_id);
}
template <>
inline const Image<Vector3> &get_img(const ImageTexture<Vector3> &t, const TexturePool &pool) {
    return get_img3(pool, t.texture_id);
}

template <typename T>
using Texture = std::variant<ConstantTexture<T>, ImageTexture<T>>;
using Texture1or3 = std::variant<Texture<Real>, Texture<Vector3>>;

template <typename T>
struct eval_texture_op {
    T operator()(const ConstantTexture<T> &t) const;
    T operator()(const ImageTexture<T> &t) const;

    const PathVertex &vertex;
    const TexturePool &pool;
};
template <typename T>
T eval_texture_op<T>::operator()(const ConstantTexture<T> &t) const {
    return t.value;
}
template <typename T>
T eval_texture_op<T>::operator()(const ImageTexture<T> &t) const {
    // TODO: proper texture filtering
    const Image<T> &img = get_img(t, pool);
    Vector2 uv = vertex.uv;
    Vector2i coord{modulo(int(uv.x * img.width + 0.5f), img.width),
                   modulo(int(uv.y * img.height + 0.5f), img.height)};
    return img(coord.x, coord.y);
}

template <typename T>
T eval(const Texture<T> &texture, const PathVertex &vertex, const TexturePool &pool) {
    return std::visit(eval_texture_op<T>{vertex, pool}, texture);
}

inline ConstantTexture<Spectrum> make_constant_spectrum_texture(const Spectrum &spec) {
    return ConstantTexture<Spectrum>{spec};
}

inline ImageTexture<Spectrum> make_image_spectrum_texture(
        const std::string &texture_name,
        const fs::path &filename,
        TexturePool &pool,
        Real uscale = 1,
        Real vscale = 1) {
    return ImageTexture<Spectrum>{insert_image3(pool, texture_name, filename), uscale, vscale};
}

inline ImageTexture<Real> make_image_float_texture(
        const std::string &texture_name,
        const fs::path &filename,
        TexturePool &pool,
        Real uscale = 1,
        Real vscale = 1) {
    return ImageTexture<Real>{insert_image1(pool, texture_name, filename), uscale, vscale};
}
