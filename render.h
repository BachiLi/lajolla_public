#pragma once

#include "lajolla.h"
#include "image.h"
#include <memory>

struct Scene;

std::shared_ptr<Image3> render(const Scene &scene);
