#include "load_serialized.h"
#include "3rdparty/miniz.h"
#include "flexception.h"
#include "transform.h"
#include <fstream>
#include <iostream>

#define MTS_FILEFORMAT_VERSION_V3 0x0003
#define MTS_FILEFORMAT_VERSION_V4 0x0004

#define ZSTREAM_BUFSIZE 32768

enum ETriMeshFlags {
    EHasNormals = 0x0001,
    EHasTexcoords = 0x0002,
    EHasTangents = 0x0004,  // unused
    EHasColors = 0x0008,
    EFaceNormals = 0x0010,
    ESinglePrecision = 0x1000,
    EDoublePrecision = 0x2000
};

class ZStream {
    public:
    /// Create a new compression stream
    ZStream(std::fstream &fs);
    void read(void *ptr, size_t size);
    virtual ~ZStream();

    private:
    std::fstream &fs;
    size_t fsize;
    z_stream m_inflateStream;
    uint8_t m_inflateBuffer[ZSTREAM_BUFSIZE];
};

ZStream::ZStream(std::fstream &fs) : fs(fs) {
    std::streampos pos = fs.tellg();
    fs.seekg(0, fs.end);
    fsize = (size_t)fs.tellg();
    fs.seekg(pos, fs.beg);

    int windowBits = 15;
    m_inflateStream.zalloc = Z_NULL;
    m_inflateStream.zfree = Z_NULL;
    m_inflateStream.opaque = Z_NULL;
    m_inflateStream.avail_in = 0;
    m_inflateStream.next_in = Z_NULL;

    int retval = inflateInit2(&m_inflateStream, windowBits);
    if (retval != Z_OK) {
        Error("Could not initialize ZLIB");
    }
}

void ZStream::read(void *ptr, size_t size) {
    uint8_t *targetPtr = (uint8_t *)ptr;
    while (size > 0) {
        if (m_inflateStream.avail_in == 0) {
            size_t remaining = fsize - fs.tellg();
            m_inflateStream.next_in = m_inflateBuffer;
            m_inflateStream.avail_in = (uInt)min(remaining, sizeof(m_inflateBuffer));
            if (m_inflateStream.avail_in == 0) {
                Error("Read less data than expected");
            }

            fs.read((char *)m_inflateBuffer, m_inflateStream.avail_in);
        }

        m_inflateStream.avail_out = (uInt)size;
        m_inflateStream.next_out = targetPtr;

        int retval = inflate(&m_inflateStream, Z_NO_FLUSH);
        switch (retval) {
            case Z_STREAM_ERROR: {
                Error("inflate(): stream error!");
            }
            case Z_NEED_DICT: {
                Error("inflate(): need dictionary!");
            }
            case Z_DATA_ERROR: {
                Error("inflate(): data error!");
            }
            case Z_MEM_ERROR: {
                Error("inflate(): memory error!");
            }
        };

        size_t outputSize = size - (size_t)m_inflateStream.avail_out;
        targetPtr += outputSize;
        size -= outputSize;

        if (size > 0 && retval == Z_STREAM_END) {
            Error("inflate(): attempting to read past the end of the stream!");
        }
    }
}

ZStream::~ZStream() {
    inflateEnd(&m_inflateStream);
}

void skip_to_idx(std::fstream &fs, const short version, const size_t idx) {
    // Go to the end of the file to see how many components are there
    fs.seekg(-sizeof(uint32_t), fs.end);
    uint32_t count = 0;
    fs.read((char *)&count, sizeof(uint32_t));
    size_t offset = 0;
    if (version == MTS_FILEFORMAT_VERSION_V4) {
        fs.seekg(-sizeof(uint64_t) * (count - idx) - sizeof(uint32_t), fs.end);
        fs.read((char *)&offset, sizeof(size_t));
    } else {  // V3
        fs.seekg(-sizeof(uint32_t) * (count - idx + 1), fs.end);
        uint32_t upos = 0;
        fs.read((char *)&upos, sizeof(unsigned int));
        offset = upos;
    }
    fs.seekg(offset, fs.beg);
    // Skip the header
    fs.ignore(sizeof(short) * 2);
}

template <typename Precision>
std::vector<Vector3> load_position(ZStream &zs, int num_vertices) {
    std::vector<Vector3> vertices(num_vertices);
    for (int i = 0; i < (int)num_vertices; i++) {
        Precision x, y, z;
        zs.read(&x, sizeof(Precision));
        zs.read(&y, sizeof(Precision));
        zs.read(&z, sizeof(Precision));
        vertices[i] = Vector3{x, y, z};
    }
    return vertices;
}

template <typename Precision>
std::vector<Vector3> load_normal(ZStream &zs, int num_vertices) {
    std::vector<Vector3> normals(num_vertices);
    for (int i = 0; i < (int)normals.size(); i++) {
        Precision x, y, z;
        zs.read(&x, sizeof(Precision));
        zs.read(&y, sizeof(Precision));
        zs.read(&z, sizeof(Precision));
        normals[i] = Vector3{x, y, z};
    }
    return normals;
}

template <typename Precision>
std::vector<Vector2> load_uv(ZStream &zs, int num_vertices) {
    std::vector<Vector2> uvs(num_vertices);
    for (int i = 0; i < (int)uvs.size(); i++) {
        Precision u, v;
        zs.read(&u, sizeof(Precision));
        zs.read(&v, sizeof(Precision));
        uvs[i] = Vector2{u, v};
    }
    return uvs;
}

template <typename Precision>
std::vector<Vector3> load_color(ZStream &zs, int num_vertices) {
    std::vector<Vector3> colors(num_vertices);
    for (int i = 0; i < num_vertices; i++) {
        Precision r, g, b;
        zs.read(&r, sizeof(Precision));
        zs.read(&g, sizeof(Precision));
        zs.read(&b, sizeof(Precision));
        colors[i] = Vector3{r, g, b};
    }
    return colors;
}

TriangleMesh load_serialized(const fs::path &filename,
                             int shape_index,
                             const Matrix4x4 &to_world) {
    std::fstream fs(filename.c_str(), std::fstream::in | std::fstream::binary);
    // Format magic number, ignore it
    fs.ignore(sizeof(short));
    // Version number
    short version = 0;
    fs.read((char *)&version, sizeof(short));
    if (shape_index > 0) {
        skip_to_idx(fs, version, shape_index);
    }
    ZStream zs(fs);

    uint32_t flags;
    zs.read((char *)&flags, sizeof(uint32_t));
    std::string name;
    if (version == MTS_FILEFORMAT_VERSION_V4) {
        char c;
        while (true) {
            zs.read((char *)&c, sizeof(char));
            if (c == '\0')
                break;
            name.push_back(c);
        }
    }
    size_t vertex_count = 0;
    zs.read((char *)&vertex_count, sizeof(size_t));
    size_t triangle_count = 0;
    zs.read((char *)&triangle_count, sizeof(size_t));

    bool file_double_precision = flags & EDoublePrecision;
    // bool face_normals = flags & EFaceNormals;

    TriangleMesh mesh;
    if (file_double_precision) {
        mesh.positions = load_position<double>(zs, vertex_count);
    } else {
        mesh.positions = load_position<float>(zs, vertex_count);
    }
    for (auto &p : mesh.positions) {
        p = xform_point(to_world, p);
    }

    if (flags & EHasNormals) {
        if (file_double_precision) {
            mesh.normals = load_normal<double>(zs, vertex_count);
        } else {
            mesh.normals = load_normal<float>(zs, vertex_count);
        }
        for (auto &n : mesh.normals) {
            n = xform_normal(inverse(to_world), n);
        }
    }

    if (flags & EHasTexcoords) {
        if (file_double_precision) {
            mesh.uvs = load_uv<double>(zs, vertex_count);
        } else {
            mesh.uvs = load_uv<float>(zs, vertex_count);
        }
    }

    if (flags & EHasColors) {
        // Ignore the color attributes.
        if (file_double_precision) {
            load_color<double>(zs, vertex_count);
        } else {
            load_color<float>(zs, vertex_count);
        }
    }

    mesh.indices.resize(triangle_count);
    for (size_t i = 0; i < triangle_count; i++) {
        int i0, i1, i2;
        zs.read(&i0, sizeof(int));
        zs.read(&i1, sizeof(int));
        zs.read(&i2, sizeof(int));
        mesh.indices[i] = Vector3i{i0, i1, i2};
    }

    return mesh;
}
