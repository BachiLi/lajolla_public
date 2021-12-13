#pragma once

#include "lajolla.h"
#include <vector>

struct TableDist1D {
    std::vector<Real> pmf;
    std::vector<Real> cdf;
};

TableDist1D make_table_dist_1d(const std::vector<Real> f);
int sample(const TableDist1D &table, Real u);
Real pmf(const TableDist1D &table, int id);
