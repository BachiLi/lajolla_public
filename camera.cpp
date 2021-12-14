#include "camera.h"
#include "lajolla.h"
#include "transform.h"

#include <cmath>

Camera::Camera(const Matrix4x4 &cam_to_world,
               Real fov,
               int width, int height)
    : cam_to_world(cam_to_world),
      world_to_cam(inverse(cam_to_world)),
      width(width), height(height) {
    Real aspect = (Real)width / (Real)height;
    cam_to_sample = scale(Vector3(-Real(0.5), -Real(0.5) * aspect, Real(1.0))) *
                    translate(Vector3(-Real(1.0), -Real(1.0) / aspect, Real(0.0))) *
                    perspective(fov);
    sample_to_cam = inverse(cam_to_sample);
}

Ray sample_primary(const Camera &camera,
                   const Vector2 &screen_pos) {
    Vector3 pt = xform_point(camera.sample_to_cam, Vector3(screen_pos[0], screen_pos[1], Real(0)));
    Vector3 dir = normalize(pt);
    return Ray{xform_point(camera.cam_to_world, Vector3{0, 0, 0}),
               // the last normalize might not be necessary
               normalize(xform_vector(camera.cam_to_world, dir)),
               Real(0), infinity<Real>()};
}
