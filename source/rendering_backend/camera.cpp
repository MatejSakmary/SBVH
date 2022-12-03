#include "camera.hpp"

Camera::Camera(const CameraInfo & info) : 
    position{info.position}, front{info.front}, up{info.up},
    speed{20.0f}, pitch{0.0f}, yaw{-90.0f}, sensitivity{0.08f}
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
        up = glm::rotate(up, glm::radians(-20.0f * delta_time), front);
        break;
    case Direction::ROLL_RIGHT:
        up = glm::rotate(up, glm::radians(20.0f * delta_time), front);
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
    if (pitch < 1.0f || pitch > 179.0f ) 
    {
        return;
    }

    front = front_;
}

f32mat4x4 Camera::get_view_matrix()
{
    return glm::lookAt(position, position + front, up);
}

f32vec3 Camera::get_camera_position() const
{
    return position;
}