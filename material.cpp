#include "material.h"

Vector3 eval_material::operator()(const Lambertian &lambertian) const {
	return fmax(dot(w_light, normal), Real(0)) * lambertian.reflectance / c_PI;
}