#pragma once

#include "lajolla.h"
#include "scene.h"
#include <string>
#include <memory>

Scene parse_scene(const fs::path &filename, const RTCDevice &embree_device);
