#pragma once

#include "vector.h"

/// Convienent class used for storing a point on a surface.
/// Sometimes we will also use it for storing an infinitely far points (environment maps).
/// In that case we ignore the position and only use the normal.
struct PointAndNormal {
	Vector3 position;
	Vector3 normal;
};
