#include "camera.hpp"

Camera::Camera(const CameraInfo & info) : 
    position{info.position}, front{info.front}, up{info.up},
    speed{5.0f}, pitch{0.0f}, yaw{-90.0f}, sensitivity{0.08f}
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
        position += up * speed * delta_time;
        break;
    case Direction::DOWN:
        position -= up * speed * delta_time;
        break;
    
    default:
        DEBUG_OUT("[Camera::move_camera()] Unkown enum value");
        break;
    }
}

void Camera::update_front_vector(f32 x_offset, f32 y_offset)
{
    yaw += sensitivity * x_offset;
    pitch += sensitivity * y_offset;
	// make sure the camera is not flipping
	if (pitch > 89.0f) pitch = 89.0f;
	if (pitch < -89.0f) pitch = -89.0f;
}

f32mat4x4 Camera::get_view_matrix()
{
    f32vec3 front_;
    front_.x = glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
    front_.y = -glm::sin(glm::radians(pitch));
    front_.z = glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch));

    front = glm::normalize(front_);
    return glm::lookAt(position, position + front, up);
}