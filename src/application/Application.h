#pragma once

#include <memory>
#include <string>
#include <vector>

#include "rendering/Renderer.h"
#include "rendering/Camera.h"
#include "rendering/ImGuiLayer.h"
#include "models/SolarSystem.h"
#include "models/Planet.h"
#include "models/AsteroidRecord.h"
#include "simulation/Simulation.h"
#include "simulation/AsteroidListState.h"
#include "data/MongoEnvironment.h"
#include "data/MongoRepository.h"

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
    void PrintSelectedPlanetInfo();
    void PrintControls();
    void SaveCurrentConfig();
    void LoadNamedConfig();
    void LoadAsteroidData();
    void RenderAsteroidMarkers(float elapsedDays);
    void RenderUI();
    const Planet* FindPlanetByName(const std::string& name) const;
    glm::mat4 GetActiveViewMatrix(float elapsedDays);

    static void MouseCallback(GLFWwindow* window, double xPos, double yPos);
    static void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset);
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    GLFWwindow* m_Window;
    std::unique_ptr<Renderer> m_Renderer;
    std::unique_ptr<SolarSystem> m_SolarSystem;
    std::unique_ptr<Camera> m_Camera;
    std::unique_ptr<Simulation> m_Simulation;
    std::unique_ptr<MongoEnvironment> m_MongoEnvironment;
    std::unique_ptr<MongoRepository> m_MongoRepository;
    std::unique_ptr<ImGuiLayer> m_ImGuiLayer;
    std::vector<AsteroidRecord> m_Asteroids;
    AsteroidListState m_AsteroidListState;

    float m_LastFrameTime;
    float m_LastMouseX;
    float m_LastMouseY;
    bool m_FirstMouse;
    int m_ViewportWidth;
    int m_ViewportHeight;

    bool m_ShowOrbitLines;
    bool m_VisualScaleMode;
    int m_SelectedPlanetIndex;
    bool m_FocusMode;
};