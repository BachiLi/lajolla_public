#pragma once

#include "lajolla.h"
#include <vector>

/// TableDist1D stores a tabular discrete distribution
/// that we can sample from using the functions below.
/// Useful for light source sampling.
struct TableDist1D {
    std::vector<Real> pmf;
    std::vector<Real> cdf;
};

/// Construct the tabular discrete distribution given a vector of positive numbers.
TableDist1D make_table_dist_1d(const std::vector<Real> &f);

/// Sample an entry from the discrete table given a random number u \in [0, 1]
int sample(const TableDist1D &table, Real u);

/// The probability mass function of the sampling procedure above.
Real pmf(const TableDist1D &table, int id);
