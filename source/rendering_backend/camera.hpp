#pragma once

#include <string>

#include "../types.hpp"
#include "../utils.hpp"

struct CameraInfo
{
    f32vec3 position;
    f32vec3 front;
    f32vec3 up;
    f32 aspect_ratio;
    f32 fov;
};

struct Camera
{
    f32 aspect_ratio;
    f32 fov;

    explicit Camera(const CameraInfo & info);

    void move_camera(f32 delta_time, Direction direction);
    void update_front_vector(f32 x_offset, f32 y_offset);
    void set_info(const CameraInfo & info);
    void parse_view_file(const std::string file_path);
    [[nodiscard]] auto get_camera_position() const -> f32vec3;
    [[nodiscard]] auto get_view_matrix() const -> f32mat4x4;
    [[nodiscard]] auto get_ray(u32vec2 screen_coords, u32vec2 resolution) const -> Ray;

    private:
        f32vec3 position;
        f32vec3 front;
        f32vec3 up;
        f32 pitch;
        f32 yaw;
        f32 roll;
        f32 speed;
        f32 sensitivity;
        f32 roll_sensitivity;
};