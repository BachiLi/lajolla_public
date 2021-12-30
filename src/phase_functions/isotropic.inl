Spectrum eval_op::operator()(const IsotropicPhase &) const {
    return make_const_spectrum(c_INVFOURPI);
}

std::optional<Vector3> sample_phase_function_op::operator()(const IsotropicPhase &) const {
    // Uniform sphere sampling
    Real z = 1 - 2 * rnd_param.x;
    Real r = sqrt(fmax(Real(0), 1 - z * z));
    Real phi = 2 * c_PI * rnd_param.y;
    return Vector3{r * cos(phi), r * sin(phi), z};
}

Real pdf_sample_phase_op::operator()(const IsotropicPhase &) const {
    return c_INVFOURPI;
}
