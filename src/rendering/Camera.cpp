#include "rendering/Camera.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

namespace
{
    const float DefaultYaw = -90.0f;
    const float DefaultPitch = 0.0f;
    const float DefaultSpeed = 12.0f;
    const float DefaultSensitivity = 0.1f;
    const float DefaultFov = 45.0f;
    const float MinFov = 5.0f;
    const float MaxFov = 90.0f;
    const float MaxPitch = 89.0f;
}

Camera::Camera(const glm::vec3& position)
    : m_Position(position)
    , m_Front(glm::vec3(0.0f, 0.0f, -1.0f))
    , m_Up(glm::vec3(0.0f, 1.0f, 0.0f))
    , m_WorldUp(glm::vec3(0.0f, 1.0f, 0.0f))
    , m_Yaw(DefaultYaw)
    , m_Pitch(DefaultPitch)
    , m_MovementSpeed(DefaultSpeed)
    , m_MouseSensitivity(DefaultSensitivity)
    , m_FieldOfView(DefaultFov)
{
    UpdateVectors();
}

void Camera::ProcessKeyboard(GLFWwindow* window, float deltaTime)
{
    float velocity = m_MovementSpeed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        m_Position += m_Front * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        m_Position -= m_Front * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        m_Position -= m_Right * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        m_Position += m_Right * velocity;
    }
}

void Camera::ProcessMouseMovement(float xOffset, float yOffset)
{
    m_Yaw += xOffset * m_MouseSensitivity;
    m_Pitch += yOffset * m_MouseSensitivity;

    m_Pitch = std::clamp(m_Pitch, -MaxPitch, MaxPitch);

    UpdateVectors();
}

void Camera::ProcessScroll(float yOffset)
{
    m_FieldOfView -= yOffset;
    m_FieldOfView = std::clamp(m_FieldOfView, MinFov, MaxFov);
}

glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(m_Position, m_Position + m_Front, m_Up);
}

float Camera::GetFieldOfView() const
{
    return m_FieldOfView;
}

void Camera::UpdateVectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.y = sin(glm::radians(m_Pitch));
    front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

    m_Front = glm::normalize(front);
    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}