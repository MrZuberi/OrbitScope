#pragma once

#include <glm/glm.hpp>

struct GLFWwindow;

class Camera
{
public:
    Camera(const glm::vec3& position);

    void ProcessKeyboard(GLFWwindow* window, float deltaTime);
    void ProcessMouseMovement(float xOffset, float yOffset);
    void ProcessScroll(float yOffset);

    glm::mat4 GetViewMatrix() const;
    float GetFieldOfView() const;

private:
    void UpdateVectors();

    glm::vec3 m_Position;
    glm::vec3 m_Front;
    glm::vec3 m_Up;
    glm::vec3 m_Right;
    glm::vec3 m_WorldUp;

    float m_Yaw;
    float m_Pitch;
    float m_MovementSpeed;
    float m_MouseSensitivity;
    float m_FieldOfView;
};