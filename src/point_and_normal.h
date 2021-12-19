#pragma once

#include "vector.h"

/// Convienent class used for storing a point on a surface.
/// Sometimes we will also use it for storing an infinitely far points (environment maps).
/// In that case the normal is the direction of the infinitely far point pointing towards the origin,
/// and position is a point on the scene bounding sphere.
struct PointAndNormal {
	Vector3 position;
	Vector3 normal;
};
