Vector2 sample_op::operator()(const Tent &filter) const {
    // Tent filter can be sampled through two parts:
    // the positive part is     k(x) = (1 - x / half_width) / normalization
    // and the negative part is k(x) = (1 + x / half_width) / normalization
    // where normalization = (half_width - half_width^2 / width)*2 = half_width
    // (the normalization term can be derived through integration:
    //  \int_{-half_width}^{half_width} k(x) = normalization)

    // We want to map a uniform number [0, 1] to the two parts of the tent
    // we first map the first half [0, 0.5] to a uniform number u \in [0, 1]
    // then we integrate the negative part (1/(h/2)) \int_{-h}^{x} k(x) dx
    // and we get (1/(h/2)) * (x^2/(2h) + x + h/2), h is the half_width
    // inverting x^2/(2h) + x + h/2 = u * h / 2 
    //           (massaging it to x^2 + 2hx + h^2 - u h^2 = 0)
    // we get x = (-2h + sqrt(4h^2 - 4 (h^2 - u h^2))) / 2
    //          = -h + h sqrt(u)
    // (the negative sqrt produces x out of bounds)
    // note that when u = 0, x = -h, when u = 1, x = 0

    // for the positive part, we map [0.5, 1] to [0, 1] again
    // the integral is
    // (1/(h/2)) * (x - x^2/(2h))
    // and inverting (-x^2 + 2hx- v h^2 = 0) we get
    // x = (-2h + sqrt(4h^2 - 4 v h^2))) / (-2)
    //   = h - h sqrt(1 - v)
    // when v = 0, x = 0, v = 1, x = h

    Real h = filter.width / 2;
    Real x = rnd_param[0] < 0.5 ? 
        h * (sqrt(2 * rnd_param[0]) - 1) :
        h * (1 - sqrt(1 - 2 * (rnd_param[0] - Real(0.5))));
    Real y = rnd_param[1] < 0.5 ? 
        h * (sqrt(2 * rnd_param[1]) - 1) :
        h * (1 - sqrt(1 - 2 * (rnd_param[1] - Real(0.5))));
    return Vector2{x, y};
}
