#include "application/Application.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "models/Planet.h"

#include <iostream>

Application::Application()
    : m_Window(nullptr)
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

    m_Window = glfwCreateWindow(1280, 720, "OrbitScope", nullptr, nullptr);
    if (!m_Window)
    {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_Window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        return false;
    }

    glViewport(0, 0, 1280, 720);
    glEnable(GL_DEPTH_TEST);

    m_Renderer = std::make_unique<Renderer>();
    m_SolarSystem = std::make_unique<SolarSystem>();

    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 12.0f, 28.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 200.0f);
    m_Renderer->SetViewProjection(view, projection);

    return true;
}

void Application::ProcessFrame()
{
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
    m_SolarSystem.reset();
    m_Renderer.reset();

    if (m_Window)
    {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }

    glfwTerminate();
}