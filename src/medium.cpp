#include "medium.h"

struct sample_distance_op {
    Real operator()(const HomogeneousMedium &m);

    const Ray &ray;
    pcg32_state &rng;
};

#include "media/homogeneous.inl"

Real sample_distance(const Medium &m, const Ray &ray, pcg32_state &rng) {
    return std::visit(sample_distance_op{ray, rng}, m);
}
