#include "application/Application.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "models/Planet.h"

#include <iostream>

Application::Application()
    : m_Window(nullptr)
    , m_LastFrameTime(0.0f)
    , m_LastMouseX(0.0f)
    , m_LastMouseY(0.0f)
    , m_FirstMouse(true)
    , m_ViewportWidth(1280)
    , m_ViewportHeight(720)
{
}

Application::~Application()
{
    Shutdown();
}

bool Application::Initialize()
{
    if (!glfwInit())
    {
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_Window = glfwCreateWindow(m_ViewportWidth, m_ViewportHeight, "OrbitScope", nullptr, nullptr);
    if (!m_Window)
    {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_Window);
    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetWindowUserPointer(m_Window, this);
    glfwSetCursorPosCallback(m_Window, MouseCallback);
    glfwSetScrollCallback(m_Window, ScrollCallback);
    glfwSetFramebufferSizeCallback(m_Window, FramebufferSizeCallback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        return false;
    }

    glViewport(0, 0, m_ViewportWidth, m_ViewportHeight);
    glEnable(GL_DEPTH_TEST);

    m_Renderer = std::make_unique<Renderer>();
    m_SolarSystem = std::make_unique<SolarSystem>();
    m_Camera = std::make_unique<Camera>(glm::vec3(0.0f, 12.0f, 28.0f));

    return true;
}

void Application::UpdateProjection()
{
    glm::mat4 projection = glm::perspective(
        glm::radians(m_Camera->GetFieldOfView()),
        static_cast<float>(m_ViewportWidth) / static_cast<float>(m_ViewportHeight),
        0.1f,
        200.0f
    );

    m_Renderer->SetViewProjection(m_Camera->GetViewMatrix(), projection);
}

void Application::ProcessFrame()
{
    float currentTime = static_cast<float>(glfwGetTime());
    float deltaTime = currentTime - m_LastFrameTime;
    m_LastFrameTime = currentTime;

    if (glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(m_Window, true);
    }

    m_Camera->ProcessKeyboard(m_Window, deltaTime);
    UpdateProjection();

    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (const Planet& planet : m_SolarSystem->GetPlanets())
    {
        m_Renderer->DrawSphere(planet.GetModelMatrix(), planet.GetColor());
    }

    glfwSwapBuffers(m_Window);
    glfwPollEvents();
}

void Application::Run()
{
    if (!Initialize())
    {
        std::cerr << "Failed to initialize OrbitScope" << std::endl;
        return;
    }

    while (!glfwWindowShouldClose(m_Window))
    {
        ProcessFrame();
    }

    Shutdown();
}

void Application::Shutdown()
{
    m_Camera.reset();
    m_SolarSystem.reset();
    m_Renderer.reset();

    if (m_Window)
    {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }

    glfwTerminate();
}

void Application::MouseCallback(GLFWwindow* window, double xPos, double yPos)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

    float x = static_cast<float>(xPos);
    float y = static_cast<float>(yPos);

    if (app->m_FirstMouse)
    {
        app->m_LastMouseX = x;
        app->m_LastMouseY = y;
        app->m_FirstMouse = false;
    }

    float xOffset = x - app->m_LastMouseX;
    float yOffset = app->m_LastMouseY - y;

    app->m_LastMouseX = x;
    app->m_LastMouseY = y;

    app->m_Camera->ProcessMouseMovement(xOffset, yOffset);
}

void Application::ScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->m_Camera->ProcessScroll(static_cast<float>(yOffset));
}

void Application::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->m_ViewportWidth = width;
    app->m_ViewportHeight = height;
    glViewport(0, 0, width, height);
}