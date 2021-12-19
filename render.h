#pragma once

#include "lajolla.h"
#include "image.h"
#include <memory>

struct Scene;

Image3 render(const Scene &scene);
