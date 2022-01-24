#ifndef MJ_RENDERER_CAMERA_HPP
#define MJ_RENDERER_CAMERA_HPP

#include <glm/glm.hpp>

class camera
{
public:
    static constexpr float Q_ERROR_TOLERANCE = 1e-3f;
    static constexpr float Q_CORRECTION_MIN_Y = 0.5f;
public:
    camera(float aspect,
           glm::vec3 const &pos = glm::vec3(0.0f, 0.0f, 0.0f),
           glm::vec3 const &fwd = glm::vec3(1.0f, 0.0f, 0.0f),
           glm::vec3 const &up  = glm::vec3(0.0f, 1.0f, 0.0f),
           float fov = 1.4f,
           float near = 0.1f,
           float far = 100.0f,
           float ground_level = 0.5f
    );

    glm::mat4 view_projection() const noexcept;

    void rotate(float pitch, float yaw);

    void translate(float forward, float up, float right);

private:
    glm::vec3 pos_, fwd_, up_, right_;
    glm::mat4 projection_;
    float ground_level_;
};

#endif