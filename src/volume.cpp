#include "volume.h"
#include "flexception.h"
#include <fstream>
#include <variant>

std::variant<GridVolume<Real>, GridVolume<Vector3>>
        load_volume(const fs::path &filename, int target_channel) {
    // code from https://github.com/mitsuba-renderer/mitsuba/blob/master/src/volume/gridvolume.cpp#L217
    enum EVolumeType {
        EFloat32 = 1,
        EFloat16 = 2,
        EUInt8 = 3,
        EQuantizedDirections = 4
    };

    std::fstream fs(filename.c_str(), std::fstream::in | std::fstream::binary);
    char header[3];
    fs.read(header, 3);
    if (header[0] != 'V' || header[1] != 'O' || header[2] != 'L') {
        Error(std::string("Error loading volume from a file (incorrect header). Filename:") +
              filename.string());
    }
    uint8_t version;
    fs.read((char*)&version, 1);
    if (version != 3) {
        Error(std::string("Error loading volume from a file (incorrect header). Filename:") +
              filename.string());
    }

    int type;
    fs.read((char*)&type, sizeof(int));
    if (type != EFloat32) {
        Error(std::string("Unsupported volume format (only support Float32). Filename:") +
              filename.string());
    }

    int xres, yres, zres;
    fs.read((char*)&xres, sizeof(int));
    fs.read((char*)&yres, sizeof(int));
    fs.read((char*)&zres, sizeof(int));

    int channels;
    fs.read((char*)&channels, sizeof(int));
    if (channels != 1 && channels != 3) {
        Error(std::string("Unsupported volume format (wrong number of channels). Filename:") +
              filename.string())
    }

    if (type != EFloat32) {
        Error(std::string("Unsupported volume format (not Float32). Filename:") +
              filename.string());
    }

    float xmin, ymin, zmin;
    float xmax, ymax, zmax;
    fs.read((char*)&xmin, sizeof(float));
    fs.read((char*)&ymin, sizeof(float));
    fs.read((char*)&zmin, sizeof(float));
    fs.read((char*)&xmax, sizeof(float));
    fs.read((char*)&ymax, sizeof(float));
    fs.read((char*)&zmax, sizeof(float));

    std::vector<float> raw_data(xres * yres * zres * channels, 0.f);
    fs.read((char*)raw_data.data(), sizeof(float) * xres * yres * zres * channels);

    if (target_channel == 1) {
        std::vector<Real> data(xres * yres * zres);
        Real max_data = 0;
        for (int i = 0; i < xres * yres * zres; i++) {
            data[i] = raw_data[channels * i];
            max_data = max(max_data, data[i]);
        }
        return GridVolume<Real>{
            Vector3i{xres, yres, zres},
            Vector3{xmin, ymin, zmin}, // pmin
            Vector3{xmax, ymax, zmax}, // pmax
            data,
            max_data,
            Real(1)}; // scale
    } else {
        assert(target_channel == 3);
        std::vector<Spectrum> data(xres * yres * zres);
        Spectrum max_data = make_zero_spectrum();
        for (int i = 0; i < xres * yres * zres; i++) {
            if (channels == 1) {
                Real v = raw_data[i];
                data[i] = fromRGB(Vector3{v, v, v});
            } else {
                data[i] = fromRGB(
                    Vector3{raw_data[3 * i + 0],
                            raw_data[3 * i + 1],
                            raw_data[3 * i + 2]});
            }
            max_data = max(max_data, data[i]);
        }
        return GridVolume<Spectrum>{
            Vector3i{xres, yres, zres},
            Vector3{xmin, ymin, zmin}, // pmin
            Vector3{xmax, ymax, zmax}, // pmax
            data,
            max_data,
            Real(1)}; // scale
    }
}

template <>
GridVolume<Real> load_volume_from_file(const fs::path &filename) {
    return std::get<GridVolume<Real>>(load_volume(filename, 1));
}

template <>
GridVolume<Vector3> load_volume_from_file(const fs::path &filename) {
    return std::get<GridVolume<Vector3>>(load_volume(filename, 3));
}
