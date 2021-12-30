#include "../scene.h"
#include "../volume.h"

Spectrum get_majorant_op::operator()(const HeterogeneousMedium &m) {
    const VolumeSpectrum &density_volume = scene.volumes[m.density_volume_id];
    return get_max_value(density_volume);
}

Spectrum get_sigma_s_op::operator()(const HeterogeneousMedium &m) {
    const VolumeSpectrum &density_volume = scene.volumes[m.density_volume_id];
    const VolumeSpectrum &albedo_volume = scene.volumes[m.albedo_volume_id];
    Spectrum density = lookup(density_volume, p);
    Spectrum albedo = lookup(albedo_volume, p);
    return density * albedo;
}

Spectrum get_sigma_a_op::operator()(const HeterogeneousMedium &m) {
    const VolumeSpectrum &density_volume = scene.volumes[m.density_volume_id];
    const VolumeSpectrum &albedo_volume = scene.volumes[m.albedo_volume_id];
    Spectrum density = lookup(density_volume, p);
    Spectrum albedo = lookup(albedo_volume, p);
    return density * (Real(1) - albedo);
}
