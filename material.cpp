#include "material.h"
#include "intersection.h"

Vector3 eval_material::operator()(const Lambertian &lambertian) const {
	return fmax(dot(w_light, isect.geometry_normal), Real(0)) * lambertian.reflectance / c_PI;
}