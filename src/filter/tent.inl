Vector2 sample_op::operator()(const Tent &filter) const {
    // Tent filter can be sampled through two parts:
    // the positive part is     y = (1 - x / half_width) / normalization
    // and the negative part is y = (1 + x / half_width) / normalization
    // where normalization = (1 - half_width^2 / width)*2

    // We want to map a uniform number [0, 1] to the two parts of the tent
    // we first map u \in [0, 0.5] to 
    // width * sqrt(2 * u) - half_width to get the negative part.
    // We then map v \in [0.5, 1] to
    // half_width - width * sqrt(2 - 2 * v)

    // To see the correctness of this mapping,
    // Let x = width * sqrt(2 * u) - half_width
    // so u = (x/width + 1/2)^2 / 2
    // du/dx = x / width^2 \prop x
    // Same for the other part. The mapping is also continuous everywhere.

    Real x = rnd_param[0] < 0.5 ? 
        filter.width * sqrt(max(2 * rnd_param[0], Real(0))) - filter.width / 2 :
        (filter.width / 2) - filter.width * sqrt(max(2 - 2 * rnd_param[0], Real(0)));
    Real y = rnd_param[0] < 0.5 ? 
        filter.width * sqrt(max(2 * rnd_param[1], Real(0))) - filter.width / 2 :
        (filter.width / 2) - filter.width * sqrt(max(2 - 2 * rnd_param[1], Real(0)));
    return Vector2{x, y};
}
