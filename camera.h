#pragma once

#include "lajolla.h"
#include "matrix.h"
#include "vector.h"
#include "ray.h"

/// Currently we only support a pinhole perspective camera
struct Camera {
    Camera() {}
    Camera(const Matrix4x4 &cam_to_world,
           Real fov, // in degree
           int width, int height);

    Matrix4x4 sample_to_cam, cam_to_sample;
    Matrix4x4 cam_to_world, world_to_cam;
    int width, height;
};

/// Given screen position in [0, 1] x [0, 1],
/// generate a camera ray.
Ray sample_primary(const Camera &camera,
                   const Vector2 &screen_pos);