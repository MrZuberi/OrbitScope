#pragma once

#include <memory>

#include "rendering/Renderer.h"
#include "models/SolarSystem.h"

struct GLFWwindow;

class Application
{
public:
    Application();
    ~Application();

    void Run();

private:
    bool Initialize();
    void Shutdown();
    void ProcessFrame();

    GLFWwindow* m_Window;
    std::unique_ptr<Renderer> m_Renderer;
    std::unique_ptr<SolarSystem> m_SolarSystem;
};