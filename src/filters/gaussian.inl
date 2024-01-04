Vector2 sample_op::operator()(const Gaussian &filter) const {
    // Box Muller transform
    // https://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform
    Real r = filter.stddev * sqrt(-2 * log(max(rnd_param[0], Real(1e-8))));
    return Vector2{r * cos(2 * c_PI * rnd_param[1]),
                   r * sin(2 * c_PI * rnd_param[1])};
}
