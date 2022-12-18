#include "camera.hpp"

Camera::Camera(const CameraInfo & info) : 
    position{info.position}, front{info.front}, up{info.up},
    speed{100.0f}, pitch{0.0f}, yaw{-90.0f}, sensitivity{0.08f}, 
    roll_sensitivity{20.0f}, roll{0.0f}
{
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

auto Camera::get_view_matrix() -> f32mat4x4
{
    return glm::lookAt(position, position + front, up);
}

auto Camera::get_camera_position() const -> f32vec3
{
    return position;
}