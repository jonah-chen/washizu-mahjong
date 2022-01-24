#define GLM_FORCE_RADIANS

#include "camera.hpp"
#include <glm/gtc/matrix_transform.hpp>


camera::camera(float aspect,
               glm::vec3 const &pos,
               glm::vec3 const &fwd,
               glm::vec3 const &up,
               float fov,
               float near,
               float far,
               float ground_level)
    : pos_(pos),
      fwd_(glm::normalize(fwd)),
      up_(glm::normalize(up)),
      right_(glm::cross(fwd_, up_)),
      projection_(glm::perspective(fov, aspect, near, far)),
      ground_level_(ground_level)
{}

glm::mat4 camera::view_projection() const noexcept
{
    return projection_ * glm::lookAt(pos_, pos_ + fwd_, up_);
}

void camera::rotate(float pitch, float yaw)
{
    glm::mat4 rotation = glm::mat4(1.0f);
    rotation = glm::rotate(rotation, pitch, glm::vec3(right_.x, 0.f, right_.z));
    rotation = glm::rotate(rotation, yaw, glm::vec3(0.f, 1.f, 0.f));

    if ((right_.y > Q_ERROR_TOLERANCE || right_.y < -Q_ERROR_TOLERANCE) && up_.y > Q_CORRECTION_MIN_Y)
        rotation = glm::rotate(rotation, right_.y, glm::vec3(fwd_.x, 0.f, fwd_.z));

    fwd_ = glm::normalize(glm::vec3(rotation * glm::vec4(fwd_, 1.f)));
    up_ = glm::normalize(glm::vec3(rotation * glm::vec4(up_, 1.f)));
    right_ = glm::cross(fwd_, up_);
}

void camera::translate(float fwd, float up, float right)
{
    pos_.x += fwd * fwd_.x + right * right_.x;
    pos_.y += up;
    pos_.z += fwd * fwd_.z + right * right_.z;
}
