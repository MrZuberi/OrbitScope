// Implements the Application class that initializes the window, handles input, manages the simulation, rendering, and UI
#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "rendering/Renderer.h"
#include "rendering/Camera.h"
#include "rendering/ImGuiLayer.h"
#include "rendering/Texture.h"
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
    void PrintControls();
    void LoadPlanetTextures();
    void EnableAsteroids();
    void AsteroidLoadWorker();
    void PollAsteroidLoad();
    void RecenterCamera(float elapsedDays, double currentJulianDate);
    void DrawPlanetWithRing(const Planet& planet, const glm::vec3& position, float elapsedDays, size_t planetIndex);
    void RenderAsteroidMarkers(float elapsedDays, double currentJulianDate);
    void RenderUI(const glm::mat4& view, const glm::mat4& projection, float elapsedDays);
    const Planet* FindPlanetByName(const std::string& name) const;

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
    std::vector<std::unique_ptr<Texture>> m_PlanetTextures;
    std::unique_ptr<Texture> m_SaturnRingTexture;
    std::unique_ptr<Texture> m_AsteroidTexture;

    std::thread m_AsteroidLoadThread;
    std::atomic<bool> m_AsteroidLoadInProgress;
    std::atomic<bool> m_AsteroidLoadComplete;
    std::mutex m_AsteroidStagingMutex;
    std::vector<AsteroidRecord> m_StagedAsteroids;
    std::vector<std::string> m_StagedPlanetNames;

    float m_LastFrameTime;
    float m_LastMouseX;
    float m_LastMouseY;
    bool m_FirstMouse;
    int m_ViewportWidth;
    int m_ViewportHeight;

    bool m_ShowOrbitLines;
    bool m_AsteroidsEnabled;
    bool m_AsteroidsLoaded;
    bool m_ViewingAsteroid;
    bool m_NeedsRecenter;
    bool m_CursorLocked;
};