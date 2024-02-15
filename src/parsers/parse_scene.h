#pragma once

#include "lajolla.h"
#include "scene.h"
#include <string>
#include <memory>

/// Parse Mitsuba's XML scene format.
std::unique_ptr<Scene> parse_scene(const fs::path &filename, const RTCDevice &embree_device);
