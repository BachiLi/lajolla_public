#include "material.h"
#include "intersection.h"

struct eval_op {
    Vector3 operator()(const Lambertian &lambertian) const;

    const Vector3 &dir_light;
    const Vector3 &dir_view;
    const Intersection &isect;
};

Vector3 eval_op::operator()(const Lambertian &lambertian) const {
	return fmax(dot(dir_light, isect.geometry_normal), Real(0)) * lambertian.reflectance / c_PI;
}

Vector3 eval(const Material &material,
             const Vector3 &dir_light,
             const Vector3 &dir_view,
             const Intersection &isect) {
	return std::visit(eval_op{dir_light, dir_view, isect}, material);
}
