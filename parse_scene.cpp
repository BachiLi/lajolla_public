#include "parse_scene.h"
#include "flexception.h"
#include "parse_obj.h"
#include "pugixml.hpp"
#include "transform.h"
#include <cctype>
#include <map>
#include <regex>

const Real c_default_fov = 45.0;
const int c_default_res = 256;
const std::string c_default_filename = "image.pfm";
const std::string c_default_material_name = "___default_white";

std::string to_lowercase(const std::string &s) {
    std::string out = s;
    std::transform(s.begin(), s.end(), out.begin(), ::tolower);
    return out;
}

std::vector<std::string> split_string(const std::string &str, const std::regex &delim_regex) {
    std::sregex_token_iterator first{begin(str), end(str), delim_regex, -1}, last;
    std::vector<std::string> list{first, last};
    return list;
}

Vector3 parse_vector3(const std::string &value) {
    std::vector<std::string> list = split_string(value, std::regex("(,| )+"));
    Vector3 v;
    if (list.size() == 1) {
        v[0] = stof(list[0]);
        v[1] = stof(list[0]);
        v[2] = stof(list[0]);
    } else if (list.size() == 3) {
        v[0] = stof(list[0]);
        v[1] = stof(list[1]);
        v[2] = stof(list[2]);
    } else {
        Error("parse_vector3 failed");
    }
    return v;
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

Matrix4x4 parse_transform(pugi::xml_node node) {
    Matrix4x4 tform = Matrix4x4::identity();
    for (auto child : node.children()) {
        std::string name = to_lowercase(child.name());
        if (name == "scale") {
            Real x = 1.0;
            Real y = 1.0;
            Real z = 1.0;
            if (!child.attribute("x").empty())
                x = std::stof(child.attribute("x").value());
            if (!child.attribute("y").empty())
                y = std::stof(child.attribute("y").value());
            if (!child.attribute("z").empty())
                z = std::stof(child.attribute("z").value());
            tform = scale(Vector3{x, y, z}) * tform;
        } else if (name == "translate") {
            Real x = 0.0;
            Real y = 0.0;
            Real z = 0.0;
            if (!child.attribute("x").empty())
                x = std::stof(child.attribute("x").value());
            if (!child.attribute("y").empty())
                y = std::stof(child.attribute("y").value());
            if (!child.attribute("z").empty())
                z = std::stof(child.attribute("z").value());
            tform = translate(Vector3{x, y, z}) * tform;
        } else if (name == "rotate") {
            Real x = 0.0;
            Real y = 0.0;
            Real z = 0.0;
            Real angle = 0.0;
            if (!child.attribute("x").empty())
                x = std::stof(child.attribute("x").value());
            if (!child.attribute("y").empty())
                y = std::stof(child.attribute("y").value());
            if (!child.attribute("z").empty())
                z = std::stof(child.attribute("z").value());
            if (!child.attribute("angle").empty())
                angle = std::stof(child.attribute("angle").value());
            tform = rotate(angle, Vector3(x, y, z)) * tform;
        } else if (name == "lookat") {
            Vector3 pos = parse_vector3(child.attribute("origin").value());
            Vector3 target = parse_vector3(child.attribute("target").value());
            Vector3 up = parse_vector3(child.attribute("up").value());
            tform = look_at(pos, target, up) * tform;
        } else if (name == "matrix") {
            Matrix4x4 trans = parse_matrix4x4(std::string(child.attribute("value").value()));
            tform = trans * tform;
        }
    }
    return tform;
}

RenderOptions parse_integrator(pugi::xml_node node) {
    RenderOptions options;
    std::string type = node.attribute("type").value();
    if (type == "path") {
        options.integrator = Integrator::Path;
    } else if (type == "depth") {
        options.integrator = Integrator::Depth;
    } else {
        Error(std::string("Unsupported integrator: ") + type);
    }
    return options;
}

std::tuple<int /* width */, int /* height */, std::string /* filename */>
parse_film(pugi::xml_node node) {
    int width = c_default_res, height = c_default_res;
    std::string filename = c_default_filename;

    for (auto child : node.children()) {
        std::string type = child.name();
        std::string name = child.attribute("name").value();
        if (name == "width") {
            width = atoi(child.attribute("value").value());
        } else if (name == "height") {
            height = atoi(child.attribute("value").value());
        } else if (name == "filename") {
            filename = std::string(child.attribute("value").value());
        }
        if (type == "rfilter") {
            std::string filter_type = child.attribute("type").value();
            if (filter_type != "box") {
                std::cerr << "Warning: the renderer currently only supports box filters." << std::endl;
            }
        }
    }
    return std::make_tuple(width, height, filename);
}

std::tuple<Camera, std::string /* output filename */>
parse_sensor(pugi::xml_node node) {
    Real fov = c_default_fov;
    Matrix4x4 to_world = Matrix4x4::identity();
    int width = c_default_res, height = c_default_res;
    std::string filename = c_default_filename;

    std::string type = node.attribute("type").value();
    if (type == "perspective") {
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "fov") {
                fov = std::stof(child.attribute("value").value());
            } else if (name == "toWorld") {
                to_world = parse_transform(child);
            } else if (std::string(child.name()) == "film") {
                std::tie(width, height, filename) = parse_film(child);
            }
        }
    } else {
        Error(std::string("Unsupported sensor: ") + type);
    }
    return std::make_tuple(Camera(to_world, fov, width, height), filename);
}

std::tuple<std::string /* ID */, Material> parse_bsdf(pugi::xml_node node) {
    std::string type = node.attribute("type").value();
    std::string id;
    if (!node.attribute("id").empty()) {
        id = node.attribute("id").value();
    }
    if (type == "diffuse") {
        Spectrum reflectance = fromRGB(Vector3{0.5, 0.5, 0.5});
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "reflectance") {
                std::string refl_type = child.name();
                if (refl_type == "spectrum") {
                    std::vector<std::pair<Real, Real>> spec =
                        parse_spectrum(child.attribute("value").value());
                    Vector3 xyz = integrate_XYZ(spec);
                    reflectance = fromRGB(XYZ_to_RGB(xyz));
                } else {
                    Error("Unknown reflectance type.");
                }
            }
        }
        return std::make_tuple(id, Lambertian{reflectance});
    } else {
        Error("Unknown BSDF.");
    }
    return std::make_tuple("", Material{});
}

Shape parse_shape(pugi::xml_node node,
                  std::vector<Material> &materials,
                  std::map<std::string, int> &material_map,
                  std::vector<Light> &lights,
                  const std::vector<Shape> &shapes) {
    int material_id = -1;
    for (auto child : node.children()) {
        std::string name = child.name();
        if (name == "ref") {
            pugi::xml_attribute id = child.attribute("id");
            if (!id.empty()) {
                auto it = material_map.find(id.value());
                if (it == material_map.end()) {
                    Error("ref not found");
                }
                material_id = it->second;
                break;
            } else {
                Error("ref not specified");
            }
        } else if (name == "bsdf") {
            Material m;
            std::string material_name;
            std::tie(material_name, m) = parse_bsdf(child);
            if (!material_name.empty()) {
                material_id = materials.size();
                material_map[material_name] = materials.size();
                materials.push_back(m);
            }
            break;
        }
    }
    if (material_id == -1) {
        if (material_map.find(c_default_material_name) == material_map.end()) {
            material_map[c_default_material_name] = materials.size();
            materials.push_back(Lambertian{fromRGB(Vector3{0.5, 0.5, 0.5})});
        }
        material_id = material_map[c_default_material_name];
    }

    Shape shape;
    std::string type = node.attribute("type").value();
    if (type == "obj") {
        std::string filename;
        Matrix4x4 to_world = Matrix4x4::identity();
        for (auto child : node.children()) {
            std::string name = child.attribute("name").value();
            if (name == "filename") {
                filename = child.attribute("value").value();
            } else if (name == "to_world") {
                if (std::string(child.name()) == "transform") {
                    to_world = parse_transform(child);
                }
            }
        }
        shape = parse_obj(filename, to_world);
    } else {
        Error("Unknown shape");
    }
    set_material_id(shape, material_id);

    for (auto child : node.children()) {
        std::string name = child.name();
        if (name == "emitter") {
            Spectrum radiance = fromRGB(Vector3{1, 1, 1});
            for (auto grand_child : child.children()) {
                std::string name = grand_child.attribute("name").value();
                if (name == "radiance") {
                    std::string rad_type = grand_child.name();
                    if (rad_type == "spectrum") {
                        std::vector<std::pair<Real, Real>> spec =
                            parse_spectrum(grand_child.attribute("value").value());
                        Vector3 xyz = integrate_XYZ(spec);
                        radiance = fromRGB(XYZ_to_RGB(xyz));
                    }
                }
            }
            lights.push_back(DiffuseAreaLight{(int)shapes.size() /* shape ID */, radiance});
        }
    }

    return shape;
}

Scene parse_scene(pugi::xml_node node, const RTCDevice &embree_device) {
    RenderOptions options;
    Camera camera(Matrix4x4::identity(), c_default_fov, c_default_res, c_default_res);
    std::string filename = c_default_filename;
    std::vector<Material> materials;
    std::map<std::string /* name id */, int /* index id */> material_map;
    std::vector<Shape> shapes;
    std::vector<Light> lights;
    for (auto child : node.children()) {
        std::string name = child.name();
        if (name == "integrator") {
            options = parse_integrator(child);
        } else if (name == "sensor") {
            std::tie(camera, filename) = parse_sensor(child);
        } else if (name == "bsdf") {
            std::string material_name;
            Material m;
            std::tie(material_name, m) = parse_bsdf(child);
            if (!material_name.empty()) {
                material_map[material_name] = materials.size();
                materials.push_back(m);
            }
        } else if (name == "shape") {
            Shape s = parse_shape(child, materials, material_map, lights, shapes);
            shapes.push_back(s);
        }
    }
    return Scene{embree_device, camera, materials, shapes, lights, options, filename};
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
