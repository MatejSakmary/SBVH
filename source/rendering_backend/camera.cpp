#include "camera.hpp"

#include <fstream>

Camera::Camera(const CameraInfo & info) : 
    position{info.position}, front{info.front}, up{info.up}, aspect_ratio{info.aspect_ratio}, fov{info.fov},
    speed{100.0f}, pitch{0.0f}, yaw{-90.0f}, sensitivity{0.08f}, 
    roll_sensitivity{20.0f}, roll{0.0f}
{
}

void Camera::set_info(const CameraInfo & info)
{
    position = info.position;
    front = info.front;
    up = info.up;
    fov = info.fov;
}

void Camera::parse_view_file(const std::string file_path)
{
    std::ifstream file(file_path);
    std::string content;
    if(file.is_open())
    {
        std::getline(file, content);
        file.close();
    }
    else
    {
        DEBUG_OUT("[Camera::parse_view_file()] Error could not open the file at path: " << file_path);
        return;
    }

    sscanf_s(content.c_str(), "-vp %f %f %f -vd %f %f %f -vu %f %f %f -vf %f",
        &position.x, &position.y, &position.z,
        &front.x, &front.y, &front.z,
        &up.x, &up.y, &up.z,
        &fov);

    DEBUG_OUT("[Camera::parese_view_file()] Sucessfully parsed view file new camera params are: ");
    DEBUG_OUT("               position    : " << position.x << " " << position.y << " " << position.z);
    DEBUG_OUT("               direction   : " << front.x << " " << front.y << " " << front.z);
    DEBUG_OUT("               up          : " << up.x << " " << up.y << " " << up.z);
    DEBUG_OUT("               fov radians : " << fov << " fov degrees: " << glm::degrees(fov));
}

auto Camera::get_ray(u32vec2 screen_coords, u32vec2 resolution) const -> Ray
{
    f32 fov_tan = glm::tan(fov / 2.0f);
    f32vec3 right = glm::cross(front, up);
    f32vec3 right_aspect_correct = right * aspect_ratio;
    f32vec3 right_aspect_fov_correct = right_aspect_correct * fov_tan;

    f32vec3 up_fov_correct = up * fov_tan;

    f32vec3 right_offset = right_aspect_fov_correct * (2.0f * ((screen_coords.x + 0.5f) / resolution.x) - 1.0f);
    f32vec3 up_offset = up_fov_correct * (2.0f * ((screen_coords.y + 0.5f) / resolution.y) - 1.0f);

    f32vec3 direction = front + right_offset + up_offset;
    
    return Ray(position, direction);
}

void Camera::move_camera(f32 delta_time, Direction direction)
{
    switch (direction)
    {
    case Direction::FORWARD:
        position += front * speed * delta_time;
        break;
    case Direction::BACK:
        position -= front * speed * delta_time;
        break;
    case Direction::LEFT:
        position -= glm::normalize(glm::cross(front, up)) * speed * delta_time;
        break;
    case Direction::RIGHT:
        position += glm::normalize(glm::cross(front, up)) * speed * delta_time;
        break;
    case Direction::UP:
        position += glm::normalize(glm::cross(glm::cross(front,up), front)) * speed * delta_time;
        break;
    case Direction::DOWN:
        position -= glm::normalize(glm::cross(glm::cross(front,up), front)) * speed * delta_time;
        break;
    case Direction::ROLL_LEFT:
        up = glm::rotate(up, static_cast<f32>(glm::radians(-roll_sensitivity * delta_time)), front);
        break;
    case Direction::ROLL_RIGHT:
        up = glm::rotate(up, static_cast<f32>(glm::radians(roll_sensitivity * delta_time)), front);
        break;
    
    default:
        DEBUG_OUT("[Camera::move_camera()] Unkown enum value");
        break;
    }
}

void Camera::update_front_vector(f32 x_offset, f32 y_offset)
{

    f32vec3 front_ = glm::rotate(front, glm::radians(-sensitivity * x_offset), up);
    front_ = glm::rotate(front_, glm::radians(-sensitivity * y_offset), glm::cross(front,up));

    pitch = glm::degrees(glm::angle(front_, up));

    const f32 MAX_PITCH_ANGLE = 179.0f;
    const f32 MIN_PITCH_ANGLE = 1.0f;
    if (pitch < MIN_PITCH_ANGLE || pitch > MAX_PITCH_ANGLE ) 
    {
        return;
    }

    front = front_;
}

auto Camera::get_view_matrix() const -> f32mat4x4
{
    return glm::lookAt(position, position + front, up);
}

auto Camera::get_camera_position() const -> f32vec3
{
    return position;
}