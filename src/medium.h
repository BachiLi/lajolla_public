#pragma once

#include <variant>

struct HomogeneousMedium {
	Spectrum sigma_a, sigma_s;
};

using Medium = std::variant<HomogeneousMedium>;
