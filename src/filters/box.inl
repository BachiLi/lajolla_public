Vector2 sample_op::operator()(const Box &filter) const {
    // Warp [0, 1]^2 to [-width/2, width/2]^2
    return (Real(2) * rnd_param - Real(1)) * (filter.width / 2);
}
