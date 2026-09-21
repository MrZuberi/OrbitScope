#include "application/Application.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

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

    m_Renderer = std::make_unique<Renderer>();

    return true;
}

void Application::ProcessFrame()
{
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_Renderer->DrawTriangle();

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
    m_Renderer.reset();

    if (m_Window)
    {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }

    glfwTerminate();
}