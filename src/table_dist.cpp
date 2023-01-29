#include "table_dist.h"

TableDist1D make_table_dist_1d(const std::vector<Real> &f) {
    std::vector<Real> pmf = f;
    std::vector<Real> cdf(f.size() + 1);
    cdf[0] = 0;
    for (int i = 0; i < (int)f.size(); i++) {
        assert(pmf[i] >= 0);
        cdf[i + 1] = cdf[i] + pmf[i];
    }
    Real total = cdf.back();
    if (total > 0) {
        for (int i = 0; i < (int)pmf.size(); i++) {
            pmf[i] /= total;
            cdf[i] /= total;
        }
    } else {
        for (int i = 0; i < (int)pmf.size(); i++) {
            pmf[i] = Real(1) / Real(pmf.size());
            cdf[i] = Real(i) / Real(pmf.size());
        }
        cdf.back() = 1;
    }
    return TableDist1D{pmf, cdf};
}

int sample(const TableDist1D &table, Real rnd_param) {
    int size = table.pmf.size();
    assert(size > 0);
    const Real *ptr = std::upper_bound(table.cdf.data(), table.cdf.data() + size + 1, rnd_param);
    int offset = std::clamp(int(ptr - table.cdf.data() - 1), 0, size - 1);
    return offset;
}

Real pmf(const TableDist1D &table, int id) {
    assert(id >= 0 && id < (int)table.pmf.size());
    return table.pmf[id];
}

TableDist2D make_table_dist_2d(const std::vector<Real> &f, int width, int height) {
    // Construct a 1D distribution for each row
    std::vector<Real> cdf_rows(height * (width + 1));
    std::vector<Real> pdf_rows(height * width);
    for (int y = 0; y < height; y++) {
        cdf_rows[y * (width + 1)] = 0;
        for (int x = 0; x < width; x++) {
            cdf_rows[y * (width + 1) + (x + 1)] =
                cdf_rows[y * (width + 1) + x] + f[y * width + x];
        }
        Real integral = cdf_rows[y * (width + 1) + width];
        if (integral > 0) {
            // Normalize
            for (int x = 0; x < width; x++) {
                cdf_rows[y * (width + 1) + x] /= integral;
            }
            // Note that after normalization, the last entry of each row for
            // cdf_rows is still the "integral".
            // We need this later for constructing the marginal distribution.

            // Setup the pmf/pdf
            for (int x = 0; x < width; x++) {
                pdf_rows[y * width + x] = f[y * width + x] / integral;
            }
        } else {
            // We shouldn't sample this row, but just in case we
            // set up a uniform distribution.
            for (int x = 0; x < width; x++) {
                pdf_rows[y * width + x] = Real(1) / Real(width);
                cdf_rows[y * (width + 1) + x] = Real(x) / Real(width);
            }
            cdf_rows[y * (width + 1) + width] = 1;
        }
    }
    // Now construct the marginal CDF for each column.
    std::vector<Real> cdf_marginals(height + 1);
    std::vector<Real> pdf_marginals(height);
    cdf_marginals[0] = 0;
    for (int y = 0; y < height; y++) {
        Real weight = cdf_rows[y * (width + 1) + width];
        cdf_marginals[y + 1] = cdf_marginals[y] + weight;
    }
    Real total_values = cdf_marginals.back();
    if (total_values > 0) {
        // Normalize
        for (int y = 0; y < height; y++) {
            cdf_marginals[y] /= total_values;
        }
        cdf_marginals[height] = 1;
        // Setup pdf cols
        for (int y = 0; y < height; y++) {
            Real weight = cdf_rows[y * (width + 1) + width];
            pdf_marginals[y] = weight / total_values;
        }
    } else {
        // The whole thing is black...why are we even here?
        // Still set up a uniform distribution.
        for (int y = 0; y < height; y++) {
            pdf_marginals[y] = Real(1) / Real(height);
            cdf_marginals[y] = Real(y) / Real(height);
        }
        cdf_marginals[height] = 1;
    }
    // We finally normalize the last entry of each cdf row to 1
    for (int y = 0; y < height; y++) {
        cdf_rows[y * (width + 1) + width] = 1;
    }

    return TableDist2D{
        cdf_rows, pdf_rows,
        cdf_marginals, pdf_marginals,
        total_values,
        width, height
    };
}

Vector2 sample(const TableDist2D &table, const Vector2 &rnd_param) {
    int w = table.width, h = table.height;
    // We first sample a row from the marginal distribution
    const Real *y_ptr = std::upper_bound(
        table.cdf_marginals.data(),
        table.cdf_marginals.data() + h + 1,
        rnd_param[1]);
    int y_offset = std::clamp(int(y_ptr - table.cdf_marginals.data() - 1), 0, h - 1);
    // Uniformly remap rnd_param[1] to find the continuous offset 
    Real dy = rnd_param[1] - table.cdf_marginals[y_offset];
    if ((table.cdf_marginals[y_offset + 1] - table.cdf_marginals[y_offset]) > 0) {
        dy /= (table.cdf_marginals[y_offset + 1] - table.cdf_marginals[y_offset]);
    }
    // Sample a column at the row y_offset
    const Real *cdf = &table.cdf_rows[y_offset * (w + 1)];
    const Real *x_ptr = std::upper_bound(cdf, cdf + w + 1, rnd_param[0]);
    int x_offset = std::clamp(int(x_ptr - cdf - 1), 0, w - 1);
    // Uniformly remap rnd_param[0]
    Real dx = rnd_param[0] - cdf[x_offset];
    if (cdf[x_offset + 1] - cdf[x_offset] > 0) {
        dx /= (cdf[x_offset + 1] - cdf[x_offset]);
    }
    return Vector2{(x_offset + dx) / w, (y_offset + dy) / h};
}

Real pdf(const TableDist2D &table, const Vector2 &xy) {
    // Convert xy to integer rows & columns
    int w = table.width, h = table.height;
    int x = std::clamp(xy.x * w, Real(0), Real(w - 1));
    int y = std::clamp(xy.y * h, Real(0), Real(h - 1));
    // What's the PDF for sampling row y?
    Real pdf_y = table.pdf_marginals[y];
    // What's the PDF for sampling row x?
    Real pdf_x = table.pdf_rows[y * w + x];
    return pdf_y * pdf_x * w * h;
}
