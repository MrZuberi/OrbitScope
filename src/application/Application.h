#pragma once

#include <memory>

#include "rendering/Renderer.h"
#include "rendering/Camera.h"
#include "models/SolarSystem.h"
#include "simulation/Simulation.h"

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
    void UpdateProjection();
    void PrintSelectedPlanetInfo();

    static void MouseCallback(GLFWwindow* window, double xPos, double yPos);
    static void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset);
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    GLFWwindow* m_Window;
    std::unique_ptr<Renderer> m_Renderer;
    std::unique_ptr<SolarSystem> m_SolarSystem;
    std::unique_ptr<Camera> m_Camera;
    std::unique_ptr<Simulation> m_Simulation;

    float m_LastFrameTime;
    float m_LastMouseX;
    float m_LastMouseY;
    bool m_FirstMouse;
    int m_ViewportWidth;
    int m_ViewportHeight;

    bool m_ShowOrbitLines;
    bool m_VisualScaleMode;
    int m_SelectedPlanetIndex;
};