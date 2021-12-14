#include "table_dist.h"

TableDist1D make_table_dist_1d(const std::vector<Real> &f) {
	std::vector<Real> pmf = f;
    std::vector<Real> cdf(f.size() + 1);
    cdf[0] = 0;
    for (int i = 0; i < (int)f.size(); i++) {
        assert(pmf[i] > 0);
        cdf[i + 1] = cdf[i] + pmf[i];
    }
    Real total = cdf.back();
    if (total > 0) {
        for (int i = 0; i < (int)pmf.size(); i++) {
            pmf[i] /= total;
            cdf[i] /= total;
        }
    }
    return TableDist1D{pmf, cdf};
}

int sample(const TableDist1D &table, Real u) {
	int size = table.pmf.size();
    assert(size > 0);
    const Real *ptr = std::upper_bound(table.cdf.data(), table.cdf.data() + size + 1, u);
    int offset = std::clamp(int(ptr - table.cdf.data() - 1), 0, size - 1);
    return offset;
}

Real pmf(const TableDist1D &table, int id) {
    assert(id >= 0 && id < (int)table.pmf.size());
    return table.pmf[id];
}
