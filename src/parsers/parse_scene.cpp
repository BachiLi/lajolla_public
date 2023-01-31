#include "parse_scene.h"
#include "3rdparty/pugixml.hpp"
#include "flexception.h"
#include "load_serialized.h"
#include "parse_obj.h"
#include "parse_ply.h"
#include "shape_utils.h"
#include "transform.h"
#include <cctype>
#include <map>
#include <regex>

const Real c_default_fov = 45.0;
const int c_default_res = 256;
const std::string c_default_filename = "image.exr";
const Filter c_default_filter = Box{Real(1)};;

struct ParsedSampler {
    int sample_count = 4;
};

enum class TextureType {
    BITMAP,
    CHECKERBOARD
};

struct ParsedTexture {
    TextureType type;
    fs::path filename;
    Spectrum color0, color1; // for checkerboard
    Real uscale = 1, vscale = 1;
    Real uoffset = 0, voffset = 0;
};

enum class FovAxis {
    X,
    Y,
    DIAGONAL,
    SMALLER,
    LARGER
};

std::vector<std::string> split_string(const std::string &str, const std::regex &delim_regex) {
    std::sregex_token_iterator first{begin(str), end(str), delim_regex, -1}, last;
    std::vector<std::string> list{first, last};
    return list;
}

auto parse_default_map(const std::string &value,
                       const std::map<std::string, std::string> &default_map) {
    if (value.length() > 0) {
        if (value[0] == '$') {
            auto it = default_map.find(value.substr(1));
            if (it == default_map.end()) {
                Error(std::string("Reference default variable ") + value + std::string(" not found."));
            }
            return it;
        }
    }
    return default_map.end();
}

bool parse_boolean(const std::string &value) {
    if (value == "true") {
        return true;
    } else if (value == "false") {
        return false;
    } else {
        Error("parse_boolean failed");
        return false;
    }
}

bool parse_boolean(const std::string &value,
                   const std::map<std::string, std::string> &default_map) {
    if (auto it = parse_default_map(value, default_map); it != default_map.end()) {
        return parse_boolean(it->second);
    }
    return parse_boolean(value);    
}

int parse_integer(const std::string &value,
                  const std::map<std::string, std::string> &default_map) {
    if (auto it = parse_default_map(value, default_map); it != default_map.end()) {
        return std::stoi(it->second);
    }
    return std::stoi(value);
}

Real parse_float(const std::string &value,
                 const std::map<std::string, std::string> &default_map) {
    if (auto it = parse_default_map(value, default_map); it != default_map.end()) {
        return std::stof(it->second);
    }
    return std::stof(value);
}

std::string parse_string(const std::string &value,
                         const std::map<std::string, std::string> &default_map) {
    if (auto it = parse_default_map(value, default_map); it != default_map.end()) {
        return it->second;
    }
    return value;
}

Vector3 parse_vector3(const std::string &value) {
    std::vector<std::string> list = split_string(value, std::regex("(,| )+"));
    Vector3 v;
    if (list.size() == 1) {
        v[0] = std::stof(list[0]);
        v[1] = std::stof(list[0]);
        v[2] = std::stof(list[0]);
    } else if (list.size() == 3) {
        v[0] = std::stof(list[0]);
        v[1] = std::stof(list[1]);
        v[2] = std::stof(list[2]);
    } else {
        Error("parse_vector3 failed");
    }
    return v;
}

Vector3 parse_vector3(const std::string &value,
                      const std::map<std::string, std::string> &default_map) {
    if (auto it = parse_default_map(value, default_map); it != default_map.end()) {
        return parse_vector3(it->second);
    }
    return parse_vector3(value);
}

Vector3 parse_srgb(const std::string &value) {
    Vector3 srgb;
    if (value.size() == 7 && value[0] == '#') {
        char *end_ptr = NULL;
        // parse hex code (#abcdef)
        int encoded = strtol(value.c_str()+1, &end_ptr, 16);
        if (*end_ptr != '\0') {
            Error(std::string("Invalid SRGB value: ") + value);
        }
        srgb[0] = ((encoded & 0xFF0000) >> 16) / 255.0f;
        srgb[1] = ((encoded & 0x00FF00) >> 8) / 255.0f;
        srgb[2] =  (encoded & 0x0000FF) / 255.0f;
    } else {
        Error(std::string("Unknown SRGB format: ") + value);
    }
    return srgb;
} 

Vector3 parse_srgb(const std::string &value,
                   const std::map<std::string, std::string> &default_map) {
    if (auto it = parse_default_map(value, default_map); it != default_map.end()) {
        return parse_srgb(it->second);
    }
    return parse_srgb(value);
}

std::vector<std::pair<Real, Real>> parse_spectrum(const std::string &value) {
    std::vector<std::string> list = split_string(value, std::regex("(,| )+"));
    std::vector<std::pair<Real, Real>> s;
    if (list.size() == 1 && list[0].find(":") == std::string::npos) {
        // a single uniform value for all wavelength
        s.push_back(std::make_pair(Real(-1), stof(list[0])));
    } else {
        for (auto val_str : list) {
            std::vector<std::string> pair = split_string(val_str, std::regex(":"));
            if (pair.size() < 2) {
                Error("parse_spectrum failed");
            }
            s.push_back(std::make_pair(Real(stof(pair[0])), Real(stof(pair[1]))));
        }
    }
    return s;
}

std::vector<std::pair<Real, Real>> parse_spectrum(
        const std::string &value,
        const std::map<std::string, std::string> &default_map) {
    if (auto it = parse_default_map(value, default_map); it != default_map.end()) {
        return parse_spectrum(it->second);
    }
    return parse_spectrum(value);
}

Matrix4x4 parse_matrix4x4(const std::string &value) {
    std::vector<std::string> list = split_string(value, std::regex("(,| )+"));
    if (list.size() != 16) {
        Error("parse_matrix4x4 failed");
    }

    Matrix4x4 m;
    int k = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            m(i, j) = std::stof(list[k++]);
        }
    }
    return m;
}

Matrix4x4 parse_matrix4x4(const std::string &value,
                          const std::map<std::string, std::string> &default_map) {
    if (auto it = parse_default_map(value, default_map); it != default_map.end()) {
        return parse_matrix4x4(it->second);
    }
    return parse_matrix4x4(value);
}

Matrix4x4 parse_transform(pugi::xml_node node,
                          const std::map<std::string, std::string> &default_map) {
    Matrix4x4 tform = Matrix4x4::identity();
    for (auto child : node.children()) {
        std::string name = to_lowercase(child.name());
        if (name == "scale") {
            Real x = 1.0;
            Real y = 1.0;
            Real z = 1.0;
            if (!child.attribute("x").empty()) {
                x = parse_float(child.attribute("x").value(), default_map);
            }
            if (!child.attribute("y").empty()) {
                y = parse_float(child.attribute("y").value(), default_map);
            }
            if (!child.attribute("z").empty()) {
                z = parse_float(child.attribute("z").value(), default_map);
            }
            if (!child.attribute("value").empty()) {
                Vector3 v = parse_vector3(
                    child.attribute("value").value(), default_map);
                x = v.x; y = v.y; z = v.z;
            }
            tform = scale(Vector3{x, y, z}) * tform;
        } else if (name == "translate") {
            Real x = 0.0;
            Real y = 0.0;
            Real z = 0.0;
            if (!child.attribute("x").empty()) {
                x = parse_float(child.attribute("x").value(), default_map);
            }
            if (!child.attribute("y").empty()) {
                y = parse_float(child.attribute("y").value(), default_map);
            }
            if (!child.attribute("z").empty()) {
                z = parse_float(child.attribute("z").value(), default_map);
            }
            if (!child.attribute("value").empty()) {
                Vector3 v = parse_vector3(
                    child.attribute("value").value(), default_map);
                x = v.x; y = v.y; z = v.z;
            }
            tform = translate(Vector3{x, y, z}) * tform;
        } else if (name == "rotate") {
            Real x = 0.0;
            Real y = 0.0;
            Real z = 0.0;
            Real angle = 0.0;
            if (!child.attribute("x").empty()) {
                x = parse_float(child.attribute("x").value(), default_map);
            }
            if (!child.attribute("y").empty()) {
                y = parse_float(child.attribute("y").value(), default_map);
            }
            if (!child.attribute("z").empty()) {
                z = parse_float(child.attribute("z").value(), default_map);
            }
            if (!child.attribute("angle").empty()) {
                angle = parse_float(child.attribute("angle").value(), default_map);
            }
            tform = rotate(angle, Vector3(x, y, z)) * tform;
        } else if (name == "lookat") {
            Vector3 pos = parse_vector3(
                child.attribute("origin").value(), default_map);
            Vector3 target = parse_vector3(
                child.attribute("target").value(), default_map);
            Vector3 up = parse_vector3(
                child.attribute("up").value(), default_map);
            tform = look_at(pos, target, up) * tform;
        } else if (name == "matrix") {
            Matrix4x4 trans = parse_matrix4x4(
                std::string(child.attribute("value").value()), default_map);
            tform = trans * tform;
        }
    }
    return tform;
}

Spectrum parse_color(pugi::xml_node node,
                     const std::map<std::string, std::string> &default_map) {
    std::string type = node.name();
    if (type == "spectrum") {
        std::vector<std::pair<Real, Real>> spec =
            parse_spectrum(node.attribute("value").value(), default_map);
        if (spec.size() > 1) {
            Vector3 xyz = integrate_XYZ(spec);
            return fromRGB(XYZ_to_RGB(xyz));
        } else if (spec.size() == 1) {
            return fromRGB(Vector3{1, 1, 1});
        } else {
            return fromRGB(Vector3{0, 0, 0});
        }
    } else if (type == "rgb") {
        return fromRGB(parse_vector3(node.attribute("value").value(), default_map));
    } else if (type == "srgb") {
        Vector3 srgb = parse_srgb(node.attribute("value").value(), default_map);
        return fromRGB(sRGB_to_RGB(srgb));
    } else if (type == "float") {
        return make_const_spectrum(parse_float(node.attribute("value").value(), default_map));
    } else {
        Error(std::string("Unknown color type:") + type);
        return make_zero_spectrum();
    }
}

/// We don't load the images to memory at this stage. Only record their names.
ParsedTexture parse_texture(pugi::xml_node node,
                            const std::map<std::string, std::string> &default_map) {
    std::string type = node.attribute("type").value();
    if (type == "bitmap") {
        std::string filename = "";
        Real uscale = 1;
        Real vscale = 1;
        Real uoffset = 0;
        Real voffset = 0;
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "filename") {
                filename = parse_string(
                    child.attribute("value").value(), default_map);
            } else if (name == "uvscale") {
                uscale = vscale = parse_float(
                    child.attribute("value").value(), default_map);
            } else if (name == "uscale") {
                uscale = parse_float(
                    child.attribute("value").value(), default_map);
            } else if (name == "vscale") {
                vscale = parse_float(
                    child.attribute("value").value(), default_map);
            } else if (name == "uoffset") {
                uoffset = parse_float(
                    child.attribute("value").value(), default_map);
            } else if (name == "voffset") {
                voffset = parse_float(
                    child.attribute("value").value(), default_map);
            }
        }
        return ParsedTexture{TextureType::BITMAP, fs::path(filename),
            make_zero_spectrum(), make_zero_spectrum(),
            uscale, vscale, uoffset, voffset};
    } else if (type == "checkerboard") {
        Spectrum color0 = fromRGB(Vector3{Real(0.4), Real(0.4), Real(0.4)});
        Spectrum color1 = fromRGB(Vector3{Real(0.2), Real(0.2), Real(0.2)});
        Real uscale = 1;
        Real vscale = 1;
        Real uoffset = 0;
        Real voffset = 0;
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "color0") {
                color0 = parse_color(child, default_map);
            } else if (name == "color1") {
                color1 = parse_color(child, default_map);
            } else if (name == "uvscale") {
                uscale = vscale = parse_float(
                    child.attribute("value").value(), default_map);
            } else if (name == "uscale") {
                uscale = parse_float(
                    child.attribute("value").value(), default_map);
            } else if (name == "vscale") {
                vscale = parse_float(
                    child.attribute("value").value(), default_map);
            } else if (name == "uoffset") {
                uoffset = parse_float(
                    child.attribute("value").value(), default_map);
            } else if (name == "voffset") {
                voffset = parse_float(
                    child.attribute("value").value(), default_map);
            }
        }
        return ParsedTexture{TextureType::CHECKERBOARD,
            "", color0, color1, uscale, vscale, uoffset, voffset};
    }
    Error(std::string("Unknown texture type: ") + type);
    return ParsedTexture{};
}

Texture<Spectrum> parse_spectrum_texture(
        pugi::xml_node node,
        const std::map<std::string /* name id */, ParsedTexture> &texture_map,
        TexturePool &texture_pool,
        const std::map<std::string, std::string> &default_map) {
    std::string type = node.name();
    if (type == "spectrum") {
        std::vector<std::pair<Real, Real>> spec =
            parse_spectrum(node.attribute("value").value(), default_map);
        if (spec.size() > 1) {
            Vector3 xyz = integrate_XYZ(spec);
            return make_constant_spectrum_texture(fromRGB(XYZ_to_RGB(xyz)));
        } else if (spec.size() == 1) {
            return make_constant_spectrum_texture(fromRGB(Vector3{1, 1, 1}));
        } else {
            return make_constant_spectrum_texture(fromRGB(Vector3{0, 0, 0}));
        }
    } else if (type == "rgb") {
        return make_constant_spectrum_texture(
            fromRGB(parse_vector3(node.attribute("value").value(), default_map)));
    } else if (type == "srgb") {
        Vector3 srgb = parse_srgb(node.attribute("value").value(), default_map);
        return make_constant_spectrum_texture(
            fromRGB(sRGB_to_RGB(srgb)));
    } else if (type == "ref") {
        // referencing a texture
        std::string ref_id = node.attribute("id").value();
        auto t_it = texture_map.find(ref_id);
        if (t_it == texture_map.end()) {
            Error(std::string("Texture not found. ID = ") + ref_id);
        }
        const ParsedTexture t = t_it->second;
        if (t.type == TextureType::BITMAP) {
            return make_image_spectrum_texture(
                ref_id, t.filename, texture_pool, t.uscale, t.vscale, t.uoffset, t.voffset);
        } else if (t.type == TextureType::CHECKERBOARD) {
            return make_checkerboard_spectrum_texture(
                t.color0, t.color1, t.uscale, t.vscale, t.uoffset, t.voffset);
        } else {
            return make_constant_spectrum_texture(fromRGB(Vector3{0, 0, 0}));
        }
    } else if (type == "texture") {
        ParsedTexture t = parse_texture(node, default_map);
        // increment ref_id_counter until we can't find the name in texture_pool
        static int ref_id_counter = 0; // should switch to atomic<int> if we parse scenes using multiple threads
        std::string tmp_ref_name = "$inline_spectrum_texture";
        while (texture_id_exists(texture_pool, tmp_ref_name + std::to_string(ref_id_counter))) {
            ref_id_counter++;
        }
        tmp_ref_name = tmp_ref_name + std::to_string(ref_id_counter);
        if (t.type == TextureType::BITMAP) {
            return make_image_spectrum_texture(
                tmp_ref_name, t.filename, texture_pool, t.uscale, t.vscale, t.uoffset, t.voffset);
        } else if (t.type == TextureType::CHECKERBOARD) {
            return make_checkerboard_spectrum_texture(
                t.color0, t.color1, t.uscale, t.vscale, t.uoffset, t.voffset);
        } else {
            return make_constant_spectrum_texture(fromRGB(Vector3{0, 0, 0}));
        }
    } else {
        Error(std::string("Unknown spectrum texture type:") + type);
        return ConstantTexture<Spectrum>{make_zero_spectrum()};
    }
}

Texture<Real> parse_float_texture(
        pugi::xml_node node,
        const std::map<std::string /* name id */, ParsedTexture> &texture_map,
        TexturePool &texture_pool,
        const std::map<std::string, std::string> &default_map) {
    std::string type = node.name();
    if (type == "ref") {
        // referencing a texture
        std::string ref_id = node.attribute("id").value();
        auto t_it = texture_map.find(ref_id);
        if (t_it == texture_map.end()) {
            Error(std::string("Texture not found. ID = ") + ref_id);
        }
        const ParsedTexture t = t_it->second;
        if (t.type == TextureType::BITMAP) {
            return make_image_float_texture(
                ref_id, t.filename, texture_pool, t.uscale, t.vscale, t.uoffset, t.voffset);
        } else if (t.type == TextureType::CHECKERBOARD) {
            return make_checkerboard_float_texture(
                avg(t.color0), avg(t.color1), t.uscale, t.vscale, t.uoffset, t.voffset);
        } else {
            return make_constant_float_texture(0);
        }
    } else if (type == "float") {
        return make_constant_float_texture(
            parse_float(node.attribute("value").value(), default_map));
    } else if (type == "texture") {
        ParsedTexture t = parse_texture(node, default_map);
        // increment ref_id_counter until we can't find the name in texture_pool
        static int ref_id_counter = 0; // should switch to atomic<int> if we parse scenes using multiple threads
        std::string tmp_ref_name = "$inline_float_texture";
        while (texture_id_exists(texture_pool, tmp_ref_name + std::to_string(ref_id_counter))) {
            ref_id_counter++;
        }
        tmp_ref_name = tmp_ref_name + std::to_string(ref_id_counter);
        if (t.type == TextureType::BITMAP) {
            return make_image_float_texture(
                tmp_ref_name, t.filename, texture_pool, t.uscale, t.vscale, t.uoffset, t.voffset);
        } else if (t.type == TextureType::CHECKERBOARD) {
            return make_checkerboard_float_texture(
                avg(t.color0), avg(t.color1), t.uscale, t.vscale, t.uoffset, t.voffset);
        } else {
            return make_constant_float_texture(0);
        }
    } else {
        Error(std::string("Unknown float texture type:") + type);
        return make_constant_float_texture(Real(0));
    }
}

Spectrum parse_intensity(pugi::xml_node node,
                         const std::map<std::string, std::string> &default_map) {
    std::string rad_type = node.name();
    if (rad_type == "spectrum") {
        std::vector<std::pair<Real, Real>> spec =
            parse_spectrum(node.attribute("value").value(), default_map);
        if (spec.size() == 1) {
            // For a light source, the white point is
            // XYZ(0.9505, 1.0, 1.0888) instead
            // or XYZ(1, 1, 1). We need to handle this special case when
            // we don't have the full spectrum data.
            Vector3 xyz{Real(0.9505), Real(1.0), Real(1.0888)};
            return fromRGB(XYZ_to_RGB(xyz * spec[0].second));
        } else {
            Vector3 xyz = integrate_XYZ(spec);
            return fromRGB(XYZ_to_RGB(xyz));
        }
    } else if (rad_type == "rgb") {
        return fromRGB(parse_vector3(
            node.attribute("value").value(), default_map));
    } else if (rad_type == "srgb") {
        Vector3 srgb = parse_srgb(
            node.attribute("value").value(), default_map);
        return fromRGB(sRGB_to_RGB(srgb));
    }
    return make_const_spectrum(1);
}

void parse_default_map(pugi::xml_node node,
                       std::map<std::string, std::string> &default_map) {
    if (node.attribute("name")) {
        std::string name = node.attribute("name").value();
        if (node.attribute("value")) {
            std::string value = node.attribute("value").value();
            default_map[name] = value;
        }
    }
}

RenderOptions parse_integrator(pugi::xml_node node,
                               const std::map<std::string, std::string> &default_map) {
    RenderOptions options;
    std::string type = node.attribute("type").value();
    if (type == "path") {
        options.integrator = Integrator::Path;
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "maxDepth") {
                options.max_depth = parse_integer(
                    child.attribute("value").value(), default_map);
            } else if (name == "rrDepth") {
                options.rr_depth = parse_integer(
                    child.attribute("value").value(), default_map);
            }
        }
    } else if (type == "volpath") {
        options.integrator = Integrator::VolPath;
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "maxDepth" || name == "max_depth") {
                options.max_depth = parse_integer(
                    child.attribute("value").value(), default_map);
            } else if (name == "rrDepth" || name == "rr_depth") {
                options.rr_depth = parse_integer(
                    child.attribute("value").value(), default_map);
            } else if (name == "version") {
                options.vol_path_version = parse_integer(
                    child.attribute("value").value(), default_map);
            } else if (name == "maxNullCollisions" || name == "max_null_collisions") {
                options.max_null_collisions = parse_integer(
                    child.attribute("value").value(), default_map);
            }
        }
    } else if (type == "direct") {
        options.integrator = Integrator::Path;
        options.max_depth = 2;
    } else if (type == "depth") {
        options.integrator = Integrator::Depth;
    } else if (type == "shadingNormal" || type == "shading_normal") {
        options.integrator = Integrator::ShadingNormal;
    } else if (type == "meanCurvature" || type == "mean_curvature") {
        options.integrator = Integrator::MeanCurvature;
    } else if (type == "rayDifferential" || type == "ray_differential") {
        options.integrator = Integrator::RayDifferential;
    } else if (type == "mipmapLevel" || type == "mipmap_level") {
        options.integrator = Integrator::MipmapLevel;
    } else {
        Error(std::string("Unsupported integrator: ") + type);
    }
    return options;
}

std::tuple<int /* width */, int /* height */, std::string /* filename */, Filter>
        parse_film(pugi::xml_node node, const std::map<std::string, std::string> &default_map) {
    int width = c_default_res, height = c_default_res;
    std::string filename = c_default_filename;
    Filter filter = c_default_filter;

    for (auto child : node.children()) {
        std::string type = child.name();
        std::string name = child.attribute("name").value();
        if (name == "width") {
            width = parse_integer(child.attribute("value").value(), default_map);
        } else if (name == "height") {
            height = parse_integer(child.attribute("value").value(), default_map);
        } else if (name == "filename") {
            filename = parse_string(child.attribute("value").value(), default_map);
        }
        if (type == "rfilter") {
            std::string filter_type = child.attribute("type").value();
            if (filter_type == "box") {
                Real filter_width = Real(1);
                for (auto grand_child : child.children()) {
                    if (std::string(grand_child.attribute("name").value()) == "width") {
                        filter_width = parse_float(
                            grand_child.attribute("value").value(), default_map);
                    }
                }
                filter = Box{filter_width};
            } else if (filter_type == "tent") {
                Real filter_width = Real(2);
                for (auto grand_child : child.children()) {
                    if (std::string(grand_child.attribute("name").value()) == "width") {
                        filter_width = parse_float(
                            grand_child.attribute("value").value(), default_map);
                    }
                }
                filter = Tent{filter_width};
            } else if (filter_type == "gaussian") {
                Real filter_stddev = Real(0.5);
                for (auto grand_child : child.children()) {
                    if (std::string(grand_child.attribute("name").value()) == "stddev") {
                        filter_stddev = parse_float(
                            grand_child.attribute("value").value(), default_map);
                    }
                }
                filter = Gaussian{filter_stddev};
            }
        }
    }
    return std::make_tuple(width, height, filename, filter);
}

VolumeSpectrum parse_volume_spectrum(pugi::xml_node node,
                                     const std::map<std::string, std::string> &default_map) {
    std::string type = node.attribute("type").value();
    if (type == "constvolume") {
        Spectrum value = make_zero_spectrum();
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "value") {
                value = parse_color(child, default_map);
            }
        }
        return ConstantVolume<Spectrum>{value};
    } else if (type == "gridvolume") {
        std::string filename;
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "filename") {
                filename = parse_string(
                    child.attribute("value").value(), default_map);
            }
        }
        if (filename.empty()) {
            Error("Empty filename for a gridvolume.");
        }
        return load_volume_from_file<Spectrum>(filename);
    } else {
        Error(std::string("Unknown volume type:") + type);
    }
    return ConstantVolume<Spectrum>{make_zero_spectrum()};
}

PhaseFunction parse_phase_function(pugi::xml_node node,
                                   const std::map<std::string, std::string> &default_map) {
    std::string type = node.attribute("type").value();
    if (type == "isotropic") {
        return IsotropicPhase{};
    } else if (type == "hg") {
        Real g = 0;
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "g") {
                g = parse_float(child.attribute("value").value(), default_map);
            }
        }
        return HenyeyGreenstein{g};
    } else {
        Error(std::string("Unrecognized phase function:") + type);
    }
    return IsotropicPhase{};
}

std::tuple<std::string /* ID */, Medium> parse_medium(
        pugi::xml_node node,
        const std::map<std::string, std::string> &default_map) {
    PhaseFunction phase_func = IsotropicPhase{};

    std::string type = node.attribute("type").value();
    std::string id;
    if (!node.attribute("id").empty()) {
        id = node.attribute("id").value();
    }
    if (type == "homogeneous") {
        Vector3 sigma_a{0.5, 0.5, 0.5};
        Vector3 sigma_s{0.5, 0.5, 0.5};
        Real scale = 1;
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "sigmaA" || name == "sigma_a") {
                sigma_a = parse_color(child, default_map);
            } else if (name == "sigmaS" || name == "sigma_s") {
                sigma_s = parse_color(child, default_map);
            } else if (name == "scale") {
                scale = parse_float(child.attribute("value").value(),
                                    default_map);
            } else if (std::string(child.name()) == "phase") {
                phase_func = parse_phase_function(child, default_map);
            }
        }
        return std::make_tuple(id,
            HomogeneousMedium{{phase_func}, sigma_a * scale, sigma_s * scale});
    } else if (type == "heterogeneous") {
        VolumeSpectrum albedo = ConstantVolume<Spectrum>{make_const_spectrum(1)};
        VolumeSpectrum density = ConstantVolume<Spectrum>{make_const_spectrum(1)};
        Real scale = 1;
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "albedo") {
                albedo = parse_volume_spectrum(child, default_map);
            } else if (name == "density") {
                density = parse_volume_spectrum(child, default_map);
            } else if (name == "scale") {
                scale = parse_float(child.attribute("value").value(), default_map);
            } else if (std::string(child.name()) == "phase") {
                phase_func = parse_phase_function(child, default_map);
            }
        }
        // scale only applies to density!!
        set_scale(density, scale);
        return std::make_tuple(id,
            HeterogeneousMedium{{phase_func}, albedo, density});
    } else {
        Error(std::string("Unknown medium type:") + type);
    }
}

std::tuple<Camera, std::string /* output filename */, ParsedSampler>
        parse_sensor(pugi::xml_node node,
                     std::vector<Medium> &media,
                     std::map<std::string /* name id */, int /* index id */> &medium_map,
                     const std::map<std::string, std::string> &default_map) {
    Real fov = c_default_fov;
    Matrix4x4 to_world = Matrix4x4::identity();
    int width = c_default_res, height = c_default_res;
    std::string filename = c_default_filename;
    Filter filter = c_default_filter;
    FovAxis fov_axis = FovAxis::X;
    ParsedSampler sampler;
    int medium_id = -1;

    std::string type = node.attribute("type").value();
    if (type == "perspective") {
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "fov") {
                fov = parse_float(child.attribute("value").value(), default_map);
            } else if (name == "toWorld" || name == "to_world") {
                to_world = parse_transform(child, default_map);
            } else if (name == "fovAxis" || name == "fov_axis") {
                std::string value = child.attribute("value").value();
                if (value == "x") {
                    fov_axis = FovAxis::X;
                } else if (value == "y") {
                    fov_axis = FovAxis::Y;
                } else if (value == "diagonal") {
                    fov_axis = FovAxis::DIAGONAL;
                } else if (value == "smaller") {
                    fov_axis = FovAxis::SMALLER;
                } else if (value == "larger") {
                    fov_axis = FovAxis::LARGER;
                } else {
                    Error(std::string("Unknown fovAxis value: ") + value);
                }
            }
        }
    } else {
        Error(std::string("Unsupported sensor: ") + type);
    }

    for (auto child : node.children()) {
        if (std::string(child.name()) == "film") {
            std::tie(width, height, filename, filter) = parse_film(child, default_map);
        } else if (std::string(child.name()) == "sampler") {
            std::string name = child.attribute("type").value();
            if (name != "independent") {
                std::cerr << "Warning: the renderer currently only supports independent samplers." << std::endl;
            }
            for (auto grand_child : child.children()) {
                std::string name = grand_child.attribute("name").value();
                if (name == "sampleCount" || name == "sample_count") {
                    sampler.sample_count = parse_integer(
                        grand_child.attribute("value").value(), default_map);
                }
            }
        } else if (std::string(child.name()) == "ref") {
            // A reference to a medium
            pugi::xml_attribute id = child.attribute("id");
            if (id.empty()) {
                Error("Medium reference not specified.");
            }
            auto it = medium_map.find(id.value());
            if (it == medium_map.end()) {
                Error(std::string("Medium reference ") + id.value() + std::string(" not found."));
            }
            medium_id = it->second;
        } else if (std::string(child.name()) == "medium") {
            Medium m;
            std::string medium_name;
            std::tie(medium_name, m) = parse_medium(child, default_map);
            if (!medium_name.empty()) {
                medium_map[medium_name] = media.size();
            }
            std::string name_value = child.attribute("name").value();
            medium_id = media.size();
            media.push_back(m);
        }
    }

    // convert to fovX (code taken from 
    // https://github.com/mitsuba-renderer/mitsuba/blob/master/src/librender/sensor.cpp)
    if (fov_axis == FovAxis::Y ||
            (fov_axis == FovAxis::SMALLER && height < width) ||
            (fov_axis == FovAxis::LARGER && width < height)) {
        Real aspect = width / Real(height);
        fov = degrees(2 * atan(
            tan(radians(fov) / 2) * aspect));
    } else if (fov_axis == FovAxis::DIAGONAL) {
        Real aspect = width / Real(height);
        Real diagonal = 2 * tan(radians(fov) / 2);
        Real width = diagonal / sqrt(1 + 1 / (aspect * aspect));
        fov = degrees(2 * atan(width / 2));
    }

    return std::make_tuple(Camera(to_world, fov, width, height, filter, medium_id),
                           filename, sampler);
}

Texture<Real> alpha_to_roughness(pugi::xml_node node,
                                 const std::map<std::string /* name id */, ParsedTexture> &texture_map,
                                 TexturePool &texture_pool,
                                 const std::map<std::string, std::string> &default_map) {
    // Alpha in microfacet models requires special treatment since we need to convert
    // the values to roughness
    std::string type = node.name();
    if (type == "ref") {
        // referencing a texture
        std::string ref_id = node.attribute("id").value();
        auto t_it = texture_map.find(ref_id);
        if (t_it == texture_map.end()) {
            Error(std::string("Texture not found. ID = ") + ref_id);
        }
        const ParsedTexture t = t_it->second;
        if (t.type == TextureType::BITMAP) {
            Image1 alpha = imread1(t.filename);
            // Convert alpha to roughness.
            Image1 roughness_img(alpha.width, alpha.height);
            for (int i = 0; i < alpha.width * alpha.height; i++) {
                roughness_img.data[i] = sqrt(alpha.data[i]);
            }
            return make_image_float_texture(
                ref_id, roughness_img, texture_pool, t.uscale, t.vscale);
        } else if (t.type == TextureType::CHECKERBOARD) {
            Real roughness0 = sqrt(avg(t.color0));
            Real roughness1 = sqrt(avg(t.color1));
            return make_checkerboard_float_texture(
                roughness0, roughness1, t.uscale, t.vscale, t.uoffset, t.voffset);
        } else {
            return make_constant_float_texture(Real(0.1));
        }
    } else if (type == "float") {
        Real alpha = parse_float(node.attribute("value").value(), default_map);
        return make_constant_float_texture(sqrt(alpha));
    } else if (type == "texture") {
        ParsedTexture t = parse_texture(node, default_map);
        // increment ref_id_counter until we can't find the name in texture_pool
        static int ref_id_counter = 0; // should switch to atomic<int> if we parse scenes using multiple threads
        std::string tmp_ref_name = "$inline_alpha_texture";
        while (texture_id_exists(texture_pool, tmp_ref_name + std::to_string(ref_id_counter))) {
            ref_id_counter++;
        }
        tmp_ref_name = tmp_ref_name + std::to_string(ref_id_counter);
        if (t.type == TextureType::BITMAP) {
            Image1 alpha = imread1(t.filename);
            // Convert alpha to roughness.
            Image1 roughness_img(alpha.width, alpha.height);
            for (int i = 0; i < alpha.width * alpha.height; i++) {
                roughness_img.data[i] = sqrt(alpha.data[i]);
            }
            return make_image_float_texture(
                tmp_ref_name, t.filename, texture_pool, t.uscale, t.vscale, t.uoffset, t.voffset);
        } else if (t.type == TextureType::CHECKERBOARD) {
            Real roughness0 = sqrt(avg(t.color0));
            Real roughness1 = sqrt(avg(t.color1));
            return make_checkerboard_float_texture(
                roughness0, roughness1, t.uscale, t.vscale, t.uoffset, t.voffset);
        } else {
            return make_constant_float_texture(Real(0.1));
        }
    } else {
        Error(std::string("Unknown float texture type:") + type);
    }
}

std::tuple<std::string /* ID */, Material> parse_bsdf(
        pugi::xml_node node,
        const std::map<std::string /* name id */, ParsedTexture> &texture_map,
        TexturePool &texture_pool,
        const std::map<std::string, std::string> &default_map,
        const std::string &parent_id = "") {
    std::string type = node.attribute("type").value();
    std::string id = parent_id;
    if (!node.attribute("id").empty()) {
        id = node.attribute("id").value();
    }
    if (type == "twosided") {
        // In lajolla, all BSDFs are twosided.
        for (auto child : node.children()) {
            if (std::string(child.name()) == "bsdf") {
                return parse_bsdf(child, texture_map, texture_pool, default_map, id);
            }
        }
    } else if (type == "diffuse") {
        Texture<Spectrum> reflectance =
            make_constant_spectrum_texture(fromRGB(Vector3{0.5, 0.5, 0.5}));
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "reflectance") {
                reflectance = parse_spectrum_texture(
                    child, texture_map, texture_pool, default_map);
            }
        }
        return std::make_tuple(id, Lambertian{reflectance});
    } else if (type == "roughplastic" || type == "plastic") {
        Texture<Spectrum> diffuse_reflectance =
            make_constant_spectrum_texture(fromRGB(Vector3{0.5, 0.5, 0.5}));
        Texture<Spectrum> specular_reflectance =
            make_constant_spectrum_texture(fromRGB(Vector3{1, 1, 1}));
        Texture<Real> roughness = make_constant_float_texture(Real(0.1));
        if (type == "plastic") {
            // Approximate plastic materials with very small roughness
            roughness = make_constant_float_texture(Real(0.01));
        }
        Real intIOR = 1.49;
        Real extIOR = 1.000277;
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "diffuseReflectance" || name == "diffuse_reflectance") {
                diffuse_reflectance = parse_spectrum_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "specularReflectance" || name == "specular_reflectance") {
                specular_reflectance = parse_spectrum_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "alpha") {
                roughness = alpha_to_roughness(child, texture_map, texture_pool, default_map);
            } else if (name == "roughness") {
                roughness = parse_float_texture(child, texture_map, texture_pool, default_map);
            } else if (name == "intIOR" || name == "int_ior") {
                intIOR = parse_float(child.attribute("value").value(), default_map); 
            } else if (name == "extIOR" || name == "ext_ior") {
                extIOR = parse_float(child.attribute("value").value(), default_map); 
            }
        }
        return std::make_tuple(id, RoughPlastic{
            diffuse_reflectance, specular_reflectance, roughness, intIOR / extIOR});
    } else if (type == "roughdielectric" || type == "dielectric") {
        Texture<Spectrum> specular_reflectance =
            make_constant_spectrum_texture(fromRGB(Vector3{1, 1, 1}));
        Texture<Spectrum> specular_transmittance =
            make_constant_spectrum_texture(fromRGB(Vector3{1, 1, 1}));
        Texture<Real> roughness = make_constant_float_texture(Real(0.1));
        if (type == "dielectric") {
            // Approximate plastic materials with very small roughness
            roughness = make_constant_float_texture(Real(0.01));
        }
        Real intIOR = 1.5046;
        Real extIOR = 1.000277;
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "specularReflectance" || name == "specular_reflectance") {
                specular_reflectance = parse_spectrum_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "specularTransmittance" || name == "specular_transmittance") {
                specular_transmittance = parse_spectrum_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "alpha") {
                roughness = alpha_to_roughness(child, texture_map, texture_pool, default_map);
            } else if (name == "roughness") {
                roughness = parse_float_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "intIOR" || name == "int_ior") {
                intIOR = parse_float(child.attribute("value").value(), default_map);
            } else if (name == "extIOR" || name == "ext_ior") {
                extIOR = parse_float(child.attribute("value").value(), default_map); 
            }
        }
        return std::make_tuple(id, RoughDielectric{
            specular_reflectance, specular_transmittance, roughness, intIOR / extIOR});
    } else if (type == "disneydiffuse") {
        Texture<Spectrum> base_color =
            make_constant_spectrum_texture(fromRGB(Vector3{0.5, 0.5, 0.5}));
        Texture<Real> roughness = make_constant_float_texture(Real(0.5));
        Texture<Real> subsurface = make_constant_float_texture(Real(0));
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "baseColor" || name == "base_color") {
                base_color = parse_spectrum_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "roughness") {
                roughness = parse_float_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "subsurface") {
                subsurface = parse_float_texture(
                    child, texture_map, texture_pool, default_map);
            }
        }
        return std::make_tuple(id, DisneyDiffuse{base_color, roughness, subsurface});
    } else if (type == "disneymetal") {
        Texture<Spectrum> base_color =
            make_constant_spectrum_texture(fromRGB(Vector3{0.5, 0.5, 0.5}));
        Texture<Real> roughness =
            make_constant_float_texture(Real(0.5));
        Texture<Real> anisotropic =
            make_constant_float_texture(Real(0.0));
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "baseColor" || name == "base_color") {
                base_color = parse_spectrum_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "roughness") {
                roughness = parse_float_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "anisotropic") {
                anisotropic = parse_float_texture(
                    child, texture_map, texture_pool, default_map);
            }
        }
        return std::make_tuple(id, DisneyMetal{base_color, roughness, anisotropic});
    } else if (type == "disneyglass") {
        Texture<Spectrum> base_color = make_constant_spectrum_texture(fromRGB(Vector3{0.5, 0.5, 0.5}));
        Texture<Real> roughness = make_constant_float_texture(Real(0.5));
        Texture<Real> anisotropic = make_constant_float_texture(Real(0.0));
        Real eta = Real(1.5);
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "baseColor" || name == "base_color") {
                base_color = parse_spectrum_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "roughness") {
                roughness = parse_float_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "anisotropic") {
                anisotropic = parse_float_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "eta") {
                eta = parse_float(child.attribute("value").value(), default_map);
            }
        }
        return std::make_tuple(id, DisneyGlass{base_color, roughness, anisotropic, eta});
    } else if (type == "disneyclearcoat") {
        Texture<Real> clearcoat_gloss = make_constant_float_texture(Real(1.0));
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "clearcoatGloss") {
                clearcoat_gloss = parse_float_texture(
                    child, texture_map, texture_pool, default_map);
            }
        }
        return std::make_tuple(id, DisneyClearcoat{clearcoat_gloss});
    } else if (type == "disneysheen") {
        Texture<Spectrum> base_color =
            make_constant_spectrum_texture(fromRGB(Vector3{0.5, 0.5, 0.5}));
        Texture<Real> sheen_tint = make_constant_float_texture(Real(0.5));
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "baseColor" || name == "base_color") {
                base_color = parse_spectrum_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "sheenTint" || name == "sheen_tint") {
                sheen_tint = parse_float_texture(
                    child, texture_map, texture_pool, default_map);
            }
        }
        return std::make_tuple(id, DisneySheen{base_color, sheen_tint});
    } else if (type == "disneybsdf" || type == "principled") {
        Texture<Spectrum> base_color = make_constant_spectrum_texture(fromRGB(Vector3{0.5, 0.5, 0.5}));
        Texture<Real> specular_transmission = make_constant_float_texture(Real(0));
        Texture<Real> metallic = make_constant_float_texture(Real(0));
        Texture<Real> subsurface = make_constant_float_texture(Real(0));
        Texture<Real> specular = make_constant_float_texture(Real(0.5));
        Texture<Real> roughness = make_constant_float_texture(Real(0.5));
        Texture<Real> specular_tint = make_constant_float_texture(Real(0));
        Texture<Real> anisotropic = make_constant_float_texture(Real(0));
        Texture<Real> sheen = make_constant_float_texture(Real(0));
        Texture<Real> sheen_tint = make_constant_float_texture(Real(0.5));
        Texture<Real> clearcoat = make_constant_float_texture(Real(0));
        Texture<Real> clearcoat_gloss = make_constant_float_texture(Real(1));
        Real eta = Real(1.5);
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "baseColor" || name == "base_color") {
                base_color = parse_spectrum_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "specularTransmission" || name == "specular_transmission" ||
                        name == "specTrans" || name == "spec_trans") {
                specular_transmission = parse_float_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "metallic") {
                metallic = parse_float_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "subsurface") {
                subsurface = parse_float_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "specular") {
                specular = parse_float_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "roughness") {
                roughness = parse_float_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "specularTint" || name == "specular_tint" ||
                        name == "specTint" || name == "spec_tint") {
                specular_tint = parse_float_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "anisotropic") {
                anisotropic = parse_float_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "sheen") {
                sheen = parse_float_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "sheenTint" || name == "sheen_tint") {
                sheen_tint = parse_float_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "clearcoat") {
                clearcoat = parse_float_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "clearcoatGloss" || name == "clearcoat_gloss") {
                clearcoat_gloss = parse_float_texture(
                    child, texture_map, texture_pool, default_map);
            } else if (name == "eta") {
                eta = parse_float(child.attribute("value").value(), default_map);
            }
        }
        return std::make_tuple(id, DisneyBSDF{base_color,
                                              specular_transmission,
                                              metallic,
                                              subsurface,
                                              specular,
                                              roughness,
                                              specular_tint,
                                              anisotropic,
                                              sheen,
                                              sheen_tint,
                                              clearcoat,
                                              clearcoat_gloss,
                                              eta});
    } else if (type == "null") {
        // TODO: implement actual null BSDF (the ray will need to pass through the shape)
        return std::make_tuple(id, Lambertian{
            make_constant_spectrum_texture(fromRGB(Vector3{0.0, 0.0, 0.0}))});
    } else {
        Error(std::string("Unknown BSDF: ") + type);
    }
    return std::make_tuple("", Material{});
}

Shape parse_shape(pugi::xml_node node,
                  std::vector<Material> &materials,
                  std::map<std::string /* name id */, int /* index id */> &material_map,
                  const std::map<std::string /* name id */, ParsedTexture> &texture_map,
                  TexturePool &texture_pool,
                  std::vector<Medium> &media,
                  std::map<std::string /* name id */, int /* index id */> &medium_map,
                  std::vector<Light> &lights,
                  const std::vector<Shape> &shapes,
                  const std::map<std::string, std::string> &default_map) {
    int material_id = -1;
    int interior_medium_id = -1;
    int exterior_medium_id = -1;
    for (auto child : node.children()) {
        std::string name = child.name();
        if (name == "ref") {
            std::string name_value = child.attribute("name").value();
            pugi::xml_attribute id = child.attribute("id");
            if (id.empty()) {
                Error("Material/medium reference id not specified.");
            }
            if (name_value == "interior") {
                auto it = medium_map.find(id.value());
                if (it == medium_map.end()) {
                    Error(std::string("Medium reference ") + id.value() + std::string(" not found."));
                }
                interior_medium_id = it->second;
            } else if (name_value == "exterior") {
                auto it = medium_map.find(id.value());
                if (it == medium_map.end()) {
                    Error(std::string("Medium reference ") + id.value() + std::string(" not found."));
                }
                exterior_medium_id = it->second;
            } else {
                auto it = material_map.find(id.value());
                if (it == material_map.end()) {
                    Error(std::string("Material reference ") + id.value() + std::string(" not found."));
                }
                material_id = it->second;
            }
        } else if (name == "bsdf") {
            Material m;
            std::string material_name;
            std::tie(material_name, m) = parse_bsdf(
                child, texture_map, texture_pool, default_map);
            if (!material_name.empty()) {
                material_map[material_name] = materials.size();
            }
            material_id = materials.size();
            materials.push_back(m);
        } else if (name == "medium") {
            Medium m;
            std::string medium_name;
            std::tie(medium_name, m) = parse_medium(child, default_map);
            if (!medium_name.empty()) {
                medium_map[medium_name] = media.size();
            }
            std::string name_value = child.attribute("name").value();
            if (name_value == "interior") {
                interior_medium_id = media.size();
            } else if (name_value == "exterior") {
                exterior_medium_id = media.size();
            } else {
                Error(std::string("Unrecognized medium name: ") + name_value);
            }
            media.push_back(m);
        }
    }

    Shape shape;
    std::string type = node.attribute("type").value();
    if (type == "obj") {
        std::string filename;
        Matrix4x4 to_world = Matrix4x4::identity();
        bool face_normals = false;
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "filename") {
                filename = parse_string(child.attribute("value").value(), default_map);
            } else if (name == "toWorld" || name == "to_world") {
                if (std::string(child.name()) == "transform") {
                    to_world = parse_transform(child, default_map);
                }
            } else if (name == "faceNormals" || name == "face_normals") {
                face_normals = parse_boolean(
                    child.attribute("value").value(), default_map);
            }
        }
        shape = parse_obj(filename, to_world);
        TriangleMesh &mesh = std::get<TriangleMesh>(shape);
        if (face_normals) {
            mesh.normals = std::vector<Vector3>{};
        } else {
            if (mesh.normals.size() == 0) {
                mesh.normals = compute_normal(mesh.positions, mesh.indices);
            }
        }
    } else if (type == "serialized") {
        std::string filename;
        int shape_index = 0;
        Matrix4x4 to_world = Matrix4x4::identity();
        bool face_normals = false;
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "filename") {
                filename = parse_string(child.attribute("value").value(), default_map);
            } else if (name == "toWorld" || name == "to_world") {
                if (std::string(child.name()) == "transform") {
                    to_world = parse_transform(child, default_map);
                }
            } else if (name == "shapeIndex" || name == "shape_index") {
                shape_index = parse_integer(child.attribute("value").value(), default_map);
            } else if (name == "faceNormals" || name == "face_normals") {
                face_normals = parse_boolean(
                    child.attribute("value").value(), default_map);
            }
        }
        shape = load_serialized(filename, shape_index, to_world);
        TriangleMesh &mesh = std::get<TriangleMesh>(shape);
        if (face_normals) {
            mesh.normals = std::vector<Vector3>{};
        } else {
            if (mesh.normals.size() == 0) {
                mesh.normals = compute_normal(mesh.positions, mesh.indices);
            }
        }
    } else if (type == "ply") {
        std::string filename;
        int shape_index = 0;
        Matrix4x4 to_world = Matrix4x4::identity();
        bool face_normals = false;
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "filename") {
                filename = parse_string(child.attribute("value").value(), default_map);
            } else if (name == "toWorld" || name == "to_world") {
                if (std::string(child.name()) == "transform") {
                    to_world = parse_transform(child, default_map);
                }
            } else if (name == "shapeIndex" || name == "shape_index") {
                shape_index = parse_integer(child.attribute("value").value(), default_map);
            } else if (name == "faceNormals" || name == "face_normals") {
                face_normals = parse_boolean(
                    child.attribute("value").value(), default_map);
            }
        }
        shape = parse_ply(filename, to_world);
        TriangleMesh &mesh = std::get<TriangleMesh>(shape);
        if (face_normals) {
            mesh.normals = std::vector<Vector3>{};
        } else {
            if (mesh.normals.size() == 0) {
                mesh.normals = compute_normal(mesh.positions, mesh.indices);
            }
        }
    } else if (type == "sphere") {
        Vector3 center{0, 0, 0};
        Real radius = 1;
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "center") {
                center = Vector3{
                    parse_float(child.attribute("x").value(), default_map),
                    parse_float(child.attribute("y").value(), default_map),
                    parse_float(child.attribute("z").value(), default_map)};
            } else if (name == "radius") {
                radius = parse_float(child.attribute("value").value(), default_map);
            }
        }
        shape = Sphere{{}, center, radius};
    } else if (type == "rectangle") {
        Matrix4x4 to_world = Matrix4x4::identity();
        bool flip_normals = false;
        TriangleMesh mesh;
        mesh.positions = {
            Vector3{-1, -1, 0}, Vector3{ 1, -1, 0}, Vector3{ 1, 1, 0}, Vector3{-1, 1, 0}
        };
        mesh.indices = {
            Vector3i{0, 1, 2}, Vector3i{0, 2, 3}
        };
        mesh.uvs = {
            Vector2{0, 0}, Vector2{1, 0}, Vector2{1, 1}, Vector2{0, 1}
        };
        mesh.normals = {
            Vector3{0, 0, 1}, Vector3{0, 0, 1}, Vector3{0, 0, 1}, Vector3{0, 0, 1}
        };
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "toWorld" || name == "to_world") {
                if (std::string(child.name()) == "transform") {
                    to_world = parse_transform(child, default_map);
                }
            } else if (name == "flipNormals" || name == "flip_normals") {
                flip_normals = parse_boolean(child.attribute("value").value(), default_map);
            }
        }
        if (flip_normals) {
            for (auto &n : mesh.normals) {
                n = -n;
            }
        }
        for (auto &p : mesh.positions) {
            p = xform_point(to_world, p);
        }
        for (auto &n : mesh.normals) {
            n = xform_normal(inverse(to_world), n);
        }
        shape = mesh;
    } else {
        Error(std::string("Unknown shape:") + type);
    }
    set_material_id(shape, material_id);
    set_interior_medium_id(shape, interior_medium_id);
    set_exterior_medium_id(shape, exterior_medium_id);

    for (auto child : node.children()) {
        std::string name = child.name();
        if (name == "emitter") {
            Spectrum radiance = fromRGB(Vector3{1, 1, 1});
            for (auto grand_child : child.children()) {
                std::string name = grand_child.attribute("name").value();
                if (name == "radiance") {
                    radiance = parse_intensity(grand_child, default_map);
                }
            }
            set_area_light_id(shape, lights.size());
            lights.push_back(DiffuseAreaLight{(int)shapes.size() /* shape ID */, radiance});
        }
    }

    return shape;
}

Scene parse_scene(pugi::xml_node node, const RTCDevice &embree_device) {
    RenderOptions options;
    Camera camera(Matrix4x4::identity(),
                  c_default_fov,
                  c_default_res,
                  c_default_res,
                  c_default_filter,
                  -1 /*medium_id*/);
    std::string filename = c_default_filename;
    std::vector<Material> materials;
    std::map<std::string /* name id */, int /* index id */> material_map;
    TexturePool texture_pool;
    std::map<std::string /* name id */, ParsedTexture> texture_map;
    std::vector<Medium> media;
    std::map<std::string /* name id */, int /* index id */> medium_map;
    std::vector<Shape> shapes;
    std::vector<Light> lights;
    // For <default> tags
    // e.g., <default name="spp" value="4096"/> will map "spp" to "4096"
    std::map<std::string, std::string> default_map;

    int envmap_light_id = -1;
    for (auto child : node.children()) {
        std::string name = child.name();
        if (name == "default") {
            parse_default_map(child, default_map);
        } else if (name == "integrator") {
            options = parse_integrator(child, default_map);
        } else if (name == "sensor") {
            ParsedSampler sampler;
            std::tie(camera, filename, sampler) =
                parse_sensor(child, media, medium_map, default_map);
            options.samples_per_pixel = sampler.sample_count;
        } else if (name == "bsdf") {
            std::string material_name;
            Material m;
            std::tie(material_name, m) = parse_bsdf(
                child, texture_map, texture_pool, default_map);
            if (!material_name.empty()) {
                material_map[material_name] = materials.size();
                materials.push_back(m);
            }
        } else if (name == "shape") {
            Shape s = parse_shape(child,
                                  materials,
                                  material_map,
                                  texture_map,
                                  texture_pool,
                                  media,
                                  medium_map,
                                  lights,
                                  shapes,
                                  default_map);
            shapes.push_back(s);
        } else if (name == "texture") {
            std::string id = child.attribute("id").value();
            if (texture_map.find(id) != texture_map.end()) {
                Error(std::string("Duplicated texture ID:") + id);
            }
            texture_map[id] = parse_texture(child, default_map);
        } else if (name == "emitter") {
            std::string type = child.attribute("type").value();
            if (type == "envmap") {
                std::string filename;
                Real scale = 1;
                Matrix4x4 to_world = Matrix4x4::identity();
                for (auto grand_child : child.children()) {
                    std::string name = grand_child.attribute("name").value();
                    if (name == "filename") {
                        filename = parse_string(
                            grand_child.attribute("value").value(), default_map);
                    } else if (name == "toWorld" || name == "to_world") {
                        to_world = parse_transform(grand_child, default_map);
                    } else if (name == "scale") {
                        scale = parse_float(
                            grand_child.attribute("value").value(), default_map);
                    }
                }
                if (filename.size() > 0) {
                    Texture<Spectrum> t = make_image_spectrum_texture(
                        "__envmap_texture__", filename, texture_pool, 1, 1);
                    Matrix4x4 to_local = inverse(to_world);
                    lights.push_back(Envmap{t, to_world, to_local, scale});
                    envmap_light_id = (int)lights.size() - 1;
                } else {
                    Error("Filename unspecified for envmap.");
                }
            } else if (type == "point") {
                std::cout << "[Warning] converting a point light into a small spherical light." << std::endl;
                Vector3 position = Vector3{0, 0, 0};
                Spectrum intensity = make_const_spectrum(1);
                for (auto grand_child : child.children()) {
                    std::string name = grand_child.attribute("name").value();
                    if (name == "position") {
                        if (!grand_child.attribute("x").empty()) {
                            position.x = parse_float(grand_child.attribute("x").value(), default_map);
                        }
                        if (!grand_child.attribute("y").empty()) {
                            position.y = parse_float(grand_child.attribute("y").value(), default_map);
                        }
                        if (!grand_child.attribute("z").empty()) {
                            position.z = parse_float(grand_child.attribute("z").value(), default_map);
                        }
                    } else if (name == "intensity") {
                        intensity = parse_intensity(grand_child, default_map);
                    }
                }
                Shape s = Sphere{{}, position, Real(1e-4)};
                intensity *= (c_FOURPI / surface_area(s));
                Material m = Lambertian{
                    make_constant_spectrum_texture(make_zero_spectrum())};
                int material_id = materials.size();
                materials.push_back(m);
                set_material_id(s, material_id);
                set_area_light_id(s, lights.size());
                lights.push_back(DiffuseAreaLight{(int)shapes.size() /* shape ID */, intensity});
                shapes.push_back(s);
            } else if (type == "directional") {
                std::cout << "[Warning] converting a directional light into a small spherical light." << std::endl;
                Vector3 direction = Vector3{0, 0, 1};
                Spectrum intensity = make_const_spectrum(1);
                for (auto grand_child : child.children()) {
                    std::string name = grand_child.attribute("name").value();
                    if (name == "direction") {
                        if (!grand_child.attribute("x").empty()) {
                            direction.x = parse_float(grand_child.attribute("x").value(), default_map);
                        }
                        if (!grand_child.attribute("y").empty()) {
                            direction.y = parse_float(grand_child.attribute("y").value(), default_map);
                        }
                        if (!grand_child.attribute("z").empty()) {
                            direction.z = parse_float(grand_child.attribute("z").value(), default_map);
                        }
                    } else if (name == "toWorld" || name == "to_world") {
                        Matrix4x4 to_world = parse_transform(grand_child, default_map);
                        direction = xform_vector(to_world, direction);
                    } else if (name == "irradiance") {
                        intensity = parse_intensity(grand_child, default_map);
                    }
                }
                direction = normalize(direction);
                Vector3 tangent, bitangent;
                std::tie(tangent, bitangent) = coordinate_system(-direction);
                TriangleMesh mesh;
                Real length = Real(1e-3);
                Real dist = Real(1e3);
                mesh.positions = {
                    Real(0.5) * length * (-tangent-bitangent) - dist * direction,
                    Real(0.5) * length * ( tangent-bitangent) - dist * direction,
                    Real(0.5) * length * ( tangent+bitangent) - dist * direction,
                    Real(0.5) * length * (-tangent+bitangent) - dist * direction};
                mesh.indices = {
                    Vector3i{0, 1, 2}, Vector3i{0, 2, 3}
                };
                mesh.normals = {
                    direction, direction, direction, direction
                };
                intensity *= ((dist * dist) / (length * length));
                Shape s = mesh;
                Material m = Lambertian{
                    make_constant_spectrum_texture(make_zero_spectrum())};
                int material_id = materials.size();
                materials.push_back(m);
                set_material_id(s, material_id);
                set_area_light_id(s, lights.size());
                lights.push_back(DiffuseAreaLight{(int)shapes.size() /* shape ID */, intensity});
                shapes.push_back(s);
            } else {
                Error(std::string("Unknown emitter type:") + type);
            }
        } else if (name == "medium") {
            std::string medium_name;
            Medium m;
            std::tie(medium_name, m) = parse_medium(child, default_map);
            if (!medium_name.empty()) {
                medium_map[medium_name] = media.size();
                media.push_back(m);
            }
        }
    }
    return Scene{embree_device,
                 camera,
                 materials,
                 shapes,
                 lights,
                 media,
                 envmap_light_id,
                 texture_pool,
                 options,
                 filename};
}

Scene parse_scene(const fs::path &filename, const RTCDevice &embree_device) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filename.c_str());
    if (!result) {
        std::cerr << "Error description: " << result.description() << std::endl;
        std::cerr << "Error offset: " << result.offset << std::endl;
        Error("Parse error");
    }
    // back up the current working directory and switch to the parent folder of the file
    fs::path old_path = fs::current_path();
    fs::current_path(filename.parent_path());
    Scene scene = parse_scene(doc.child("scene"), embree_device);
    // switch back to the old current working directory
    fs::current_path(old_path);
    return scene;
}
