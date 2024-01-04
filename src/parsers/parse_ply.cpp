#include "parse_ply.h"
#include "flexception.h"
#include "transform.h"
#define TINYPLY_IMPLEMENTATION
#include "3rdparty/tinyply.h"

#include <fstream>

TriangleMesh parse_ply(const fs::path &filename, const Matrix4x4 &to_world) {
    std::ifstream ifs(filename, std::ios::binary);
    tinyply::PlyFile ply_file;
    ply_file.parse_header(ifs);

    std::shared_ptr<tinyply::PlyData> vertices, uvs, normals, faces;
    try {
        vertices = ply_file.request_properties_from_element("vertex", { "x", "y", "z" });
    } catch (const std::exception & e) { 
        Error(std::string("Vertex positions not found in ") + filename.string());
    }
    try {
        uvs = ply_file.request_properties_from_element("vertex", { "u", "v" });
    } catch (const std::exception & e) {
        // It's fine to not have UVs    
    }
    try {
        normals = ply_file.request_properties_from_element("vertex", { "nx", "ny", "nz" });
    } catch (const std::exception & e) {
        // It's fine to not have shading normals
    }
    try {
        faces = ply_file.request_properties_from_element("face", { "vertex_indices" }); 
    } catch (const std::exception & e) { 
        Error(std::string("Vertex indices not found in ") + filename.string());
    }

    ply_file.read(ifs);

    TriangleMesh mesh;
    mesh.positions.resize(vertices->count);
    if (vertices->t == tinyply::Type::FLOAT32) {
        float *data = (float*)vertices->buffer.get();
        for (size_t i = 0; i < vertices->count; i++) {
            mesh.positions[i] = xform_point(to_world,
                Vector3{data[3 * i], data[3 * i + 1], data[3 * i + 2]});
        }
    } else if (vertices->t == tinyply::Type::FLOAT64) {
        double *data = (double*)vertices->buffer.get();
        for (size_t i = 0; i < vertices->count; i++) {
            mesh.positions[i] = xform_point(to_world,
                Vector3{data[3 * i], data[3 * i + 1], data[3 * i + 2]});
        }
    }
    if (uvs) {
        mesh.uvs.resize(uvs->count);
        if (uvs->t == tinyply::Type::FLOAT32) {
            float *data = (float*)uvs->buffer.get();
            for (size_t i = 0; i < uvs->count; i++) {
                mesh.uvs[i] = Vector2{data[2 * i], data[2 * i + 1]};
            }
        } else if (uvs->t == tinyply::Type::FLOAT64) {
            double *data = (double*)uvs->buffer.get();
            for (size_t i = 0; i < uvs->count; i++) {
                mesh.uvs[i] = Vector2{data[2 * i], data[2 * i + 1]};
            }
        }
    }
    if (normals) {
        mesh.normals.resize(normals->count);
        if (normals->t == tinyply::Type::FLOAT32) {
            float *data = (float*)normals->buffer.get();
            for (size_t i = 0; i < normals->count; i++) {
                mesh.normals[i] = xform_normal(inverse(to_world),
                    Vector3{data[3 * i], data[3 * i + 1], data[3 * i + 2]});
            }
        } else if (normals->t == tinyply::Type::FLOAT64) {
            double *data = (double*)normals->buffer.get();
            for (size_t i = 0; i < normals->count; i++) {
                mesh.normals[i] = xform_normal(inverse(to_world),
                    Vector3{data[3 * i], data[3 * i + 1], data[3 * i + 2]});
            }
        }
    }
    mesh.indices.resize(faces->count);
    if (faces->t == tinyply::Type::INT8) {
        int8_t *data = (int8_t*)faces->buffer.get();
        for (size_t i = 0; i < faces->count; i++) {
            mesh.indices[i] = Vector3i{
                data[3 * i], data[3 * i + 1], data[3 * i + 2]};
        }
    } else if (faces->t == tinyply::Type::UINT8) {
        uint8_t *data = (uint8_t*)faces->buffer.get();
        for (size_t i = 0; i < faces->count; i++) {
            mesh.indices[i] = Vector3i{
                data[3 * i], data[3 * i + 1], data[3 * i + 2]};
        }
    } else if (faces->t == tinyply::Type::INT16) {
        int16_t *data = (int16_t*)faces->buffer.get();
        for (size_t i = 0; i < faces->count; i++) {
            mesh.indices[i] = Vector3i{
                data[3 * i], data[3 * i + 1], data[3 * i + 2]};
        }
    } else if (faces->t == tinyply::Type::UINT16) {
        uint16_t *data = (uint16_t*)faces->buffer.get();
        for (size_t i = 0; i < faces->count; i++) {
            mesh.indices[i] = Vector3i{
                data[3 * i], data[3 * i + 1], data[3 * i + 2]};
        }
    } else if (faces->t == tinyply::Type::INT32) {
        int32_t *data = (int32_t*)faces->buffer.get();
        for (size_t i = 0; i < faces->count; i++) {
            mesh.indices[i] = Vector3i{
                data[3 * i], data[3 * i + 1], data[3 * i + 2]};
        }
    } else if (faces->t == tinyply::Type::UINT32) {
        uint32_t *data = (uint32_t*)faces->buffer.get();
        for (size_t i = 0; i < faces->count; i++) {
            mesh.indices[i] = Vector3i{
                data[3 * i], data[3 * i + 1], data[3 * i + 2]};
        }
    }

    return mesh;
}
