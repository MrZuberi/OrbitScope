#include "application/Application.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "models/Planet.h"

#include <iostream>

namespace
{
    const glm::vec3 OrbitLineColor(0.4f, 0.4f, 0.4f);
    const float VisualScaleMultiplier = 3.0f;
}

Application::Application()
    : m_Window(nullptr)
    , m_LastFrameTime(0.0f)
    , m_LastMouseX(0.0f)
    , m_LastMouseY(0.0f)
    , m_FirstMouse(true)
    , m_ViewportWidth(1280)
    , m_ViewportHeight(720)
    , m_ShowOrbitLines(true)
    , m_VisualScaleMode(false)
    , m_SelectedPlanetIndex(-1)
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
    glfwSetKeyCallback(m_Window, KeyCallback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        return false;
    }

    glViewport(0, 0, m_ViewportWidth, m_ViewportHeight);
    glEnable(GL_DEPTH_TEST);

    m_Renderer = std::make_unique<Renderer>();
    m_SolarSystem = std::make_unique<SolarSystem>();
    m_Camera = std::make_unique<Camera>(glm::vec3(0.0f, 12.0f, 28.0f));
    m_Simulation = std::make_unique<Simulation>();

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
    m_Simulation->Update(deltaTime);
    UpdateProjection();

    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float elapsedDays = m_Simulation->GetElapsedDays();
    float radiusScale = m_VisualScaleMode ? VisualScaleMultiplier : 1.0f;

    const std::vector<Planet>& planets = m_SolarSystem->GetPlanets();

    for (const Planet& planet : planets)
    {
        if (m_ShowOrbitLines && planet.GetDistanceFromSun() > 0.0f)
        {
            m_Renderer->DrawOrbitLine(planet.GetOrbitModelMatrix(), OrbitLineColor);
        }
    }

    for (const Planet& planet : planets)
    {
        m_Renderer->DrawSphere(planet.GetModelMatrix(elapsedDays, radiusScale), planet.GetColor());
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
    m_Simulation.reset();
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

void Application::PrintSelectedPlanetInfo()
{
    const std::vector<Planet>& planets = m_SolarSystem->GetPlanets();

    if (m_SelectedPlanetIndex < 0 || m_SelectedPlanetIndex >= static_cast<int>(planets.size()))
    {
        return;
    }

    const Planet& planet = planets[m_SelectedPlanetIndex];

    std::cout << "Selected: " << planet.GetName() << std::endl;
    std::cout << "Radius: " << planet.GetRadius() << std::endl;
    std::cout << "Distance from Sun: " << planet.GetDistanceFromSun() << std::endl;
    std::cout << "Orbital period: " << planet.GetOrbitalPeriod() << " days" << std::endl;
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

void Application::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

    if (action != GLFW_PRESS)
    {
        return;
    }

    if (key == GLFW_KEY_SPACE)
    {
        app->m_Simulation->TogglePause();
    }
    else if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD)
    {
        app->m_Simulation->IncreaseSpeed();
    }
    else if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT)
    {
        app->m_Simulation->DecreaseSpeed();
    }
    else if (key == GLFW_KEY_R)
    {
        app->m_Simulation->Reset();
    }
    else if (key == GLFW_KEY_O)
    {
        app->m_ShowOrbitLines = !app->m_ShowOrbitLines;
    }
    else if (key == GLFW_KEY_V)
    {
        app->m_VisualScaleMode = !app->m_VisualScaleMode;
    }
    else if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
    {
        int index = key - GLFW_KEY_0;
        app->m_SelectedPlanetIndex = index;
        app->PrintSelectedPlanetInfo();
    }
}