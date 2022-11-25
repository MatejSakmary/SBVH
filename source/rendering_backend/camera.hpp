#pragma once

#include "../types.hpp"

struct CameraInfo
{
    const f32vec3 position;
    const f32vec3 front;
    const f32vec3 up;
};

struct Camera
{
    Camera(const CameraInfo & info);

    void move_camera(f32 delta_time, Direction direction);
    void update_front_vector(f32 x_offset, f32 y_offset);
    f32mat4x4 get_view_matrix();

    private:
        f32vec3 position;
        f32vec3 front;
        f32vec3 up;
        f32 pitch;
        f32 yaw;
        f32 speed;
        f32 sensitivity;
};