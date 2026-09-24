#include "application/Application.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include "data/PlanetRepository.h"
#include "data/ConfigRepository.h"
#include "data/AsteroidClient.h"
#include "data/SBDBClient.h"
#include "simulation/SimulationConfig.h"
#include "simulation/AsteroidPositioner.h"
#include "simulation/KeplerOrbitCalculator.h"

#include <cfloat>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace
{
    const glm::vec3 OrbitLineColor(0.35f, 0.35f, 0.4f);
    const double AsteroidMaxDistanceAu = 0.05;
    const int AsteroidLookAheadDays = 60;
    const float AuToUnitsScale = 110.0f;
    const float PlanetTrueScaleFactor = 1.0f / 235.0f;
    const float SunTrueScaleFactor = 1.0f / 20.0f;
}

Application::Application()
    : m_Window(nullptr)
    , m_AsteroidLoadInProgress(false)
    , m_AsteroidLoadComplete(false)
    , m_LastFrameTime(0.0f)
    , m_LastMouseX(0.0f)
    , m_LastMouseY(0.0f)
    , m_FirstMouse(true)
    , m_ViewportWidth(1280)
    , m_ViewportHeight(720)
    , m_ShowOrbitLines(true)
    , m_TrueScaleMode(false)
    , m_SelectedPlanetIndex(-1)
    , m_FocusMode(false)
    , m_PlanetFocusActive(false)
    , m_AsteroidsEnabled(false)
    , m_AsteroidsLoaded(false)
    , m_CursorLocked(true)
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

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_RED_BITS, videoMode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, videoMode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, videoMode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, videoMode->refreshRate);

    m_ViewportWidth = videoMode->width;
    m_ViewportHeight = videoMode->height;

    m_Window = glfwCreateWindow(m_ViewportWidth, m_ViewportHeight, "OrbitScope", primaryMonitor, nullptr);
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
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_Renderer = std::make_unique<Renderer>();
    m_Camera = std::make_unique<Camera>(glm::vec3(0.0f, 45.0f, 145.0f));
    m_Simulation = std::make_unique<Simulation>();
    m_ImGuiLayer = std::make_unique<ImGuiLayer>(m_Window);

    const char* mongoUri = std::getenv("MONGODB_URI");

    std::vector<PlanetRecord> records;
    bool loadedFromMongo = false;

    if (mongoUri)
    {
        m_MongoEnvironment = std::make_unique<MongoEnvironment>();
        m_MongoRepository = std::make_unique<MongoRepository>(std::string(mongoUri), "orbitscope");

        PlanetRepository planetRepository(*m_MongoRepository);

        if (planetRepository.LoadPlanets(records))
        {
            loadedFromMongo = true;
        }
        else
        {
            std::vector<PlanetRecord> defaults = SolarSystem::GetDefaultRecords();
            planetRepository.SeedPlanets(defaults);
            records = defaults;
        }
    }

    if (records.empty())
    {
        m_SolarSystem = std::make_unique<SolarSystem>();
        std::cout << "Using built in planetary data" << std::endl;
    }
    else
    {
        m_SolarSystem = std::make_unique<SolarSystem>(records);
        std::cout << (loadedFromMongo ? "Loaded planetary data from MongoDB" : "Seeded MongoDB with default planetary data") << std::endl;
    }

    LoadPlanetTextures();

    PrintControls();

    return true;
}

void Application::LoadPlanetTextures()
{
    const std::vector<Planet>& planets = m_SolarSystem->GetPlanets();

    for (const Planet& planet : planets)
    {
        m_PlanetTextures.push_back(std::make_unique<Texture>(planet.GetTexturePath()));
    }

    m_SaturnRingTexture = std::make_unique<Texture>("resources/textures/saturn_rings.png");
}

float Application::ComputeEffectiveRadius(const Planet& planet) const
{
    if (!m_TrueScaleMode)
    {
        return planet.GetRadius();
    }

    return planet.GetRadius() * (planet.IsSun() ? SunTrueScaleFactor : PlanetTrueScaleFactor);
}

void Application::EnableAsteroids()
{
    m_PlanetFocusActive = false;

    if (m_AsteroidsLoaded)
    {
        m_AsteroidsEnabled = true;
        return;
    }

    if (m_AsteroidLoadInProgress)
    {
        return;
    }

    if (m_AsteroidLoadThread.joinable())
    {
        m_AsteroidLoadThread.join();
    }

    m_AsteroidLoadInProgress = true;
    m_AsteroidLoadComplete = false;

    m_AsteroidLoadThread = std::thread(&Application::AsteroidLoadWorker, this);
}

void Application::AsteroidLoadWorker()
{
    std::vector<AsteroidRecord> asteroids;
    AsteroidClient asteroidClient;

    if (asteroidClient.FetchCloseApproaches(asteroids, AsteroidMaxDistanceAu, AsteroidLookAheadDays))
    {
        SBDBClient sbdbClient;

        for (AsteroidRecord& asteroid : asteroids)
        {
            OrbitalElements elements;

            if (sbdbClient.FetchOrbitalElements(asteroid.designation, elements))
            {
                asteroid.hasOrbitalElements = true;
                asteroid.orbitalElements = elements;
                asteroid.orbitPathPoints = KeplerOrbitCalculator::BuildOrbitPath(elements, AuToUnitsScale, 128);
            }
        }
    }

    std::vector<std::string> planetNames;

    if (FindPlanetByName("Earth"))
    {
        planetNames.push_back("Earth");
    }

    for (const Planet& planet : m_SolarSystem->GetPlanets())
    {
        if (!planet.IsSun() && planet.GetName() != "Earth")
        {
            planetNames.push_back(planet.GetName());
        }
    }

    std::lock_guard<std::mutex> lock(m_AsteroidStagingMutex);
    m_StagedAsteroids = asteroids;
    m_StagedPlanetNames = planetNames;
    m_AsteroidLoadComplete = true;
}

void Application::PollAsteroidLoad()
{
    if (!m_AsteroidLoadComplete)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_AsteroidStagingMutex);

    m_Asteroids = m_StagedAsteroids;
    m_AsteroidListState.SetSource(m_Asteroids, m_StagedPlanetNames);

    m_AsteroidsLoaded = true;
    m_AsteroidsEnabled = true;
    m_AsteroidLoadInProgress = false;
    m_AsteroidLoadComplete = false;

    std::cout << "Loaded " << m_Asteroids.size() << " asteroid close approaches" << std::endl;
}

const Planet* Application::FindPlanetByName(const std::string& name) const
{
    const std::vector<Planet>& planets = m_SolarSystem->GetPlanets();

    for (const Planet& planet : planets)
    {
        if (planet.GetName() == name)
        {
            return &planet;
        }
    }

    return nullptr;
}

glm::mat4 Application::GetActiveViewMatrix(float elapsedDays, double currentJulianDate)
{
    if (m_FocusMode && m_AsteroidListState.HasSelection())
    {
        const AsteroidRecord& selected = m_AsteroidListState.GetSelected();
        glm::vec3 markerPosition;

        if (selected.hasOrbitalElements)
        {
            markerPosition = KeplerOrbitCalculator::CalculateHeliocentricPosition(selected.orbitalElements, currentJulianDate, AuToUnitsScale);
        }
        else
        {
            const Planet* targetPlanet = FindPlanetByName(selected.targetBody);

            if (!targetPlanet)
            {
                return m_Camera->GetViewMatrix();
            }

            glm::vec3 planetPosition = targetPlanet->GetPosition(elapsedDays);
            float closeUpOffset = AsteroidPositioner::CalculateCloseUpOffset(selected.distanceAu);
            markerPosition = AsteroidPositioner::CalculateMarkerPosition(planetPosition, selected.designation, closeUpOffset);
        }

        glm::vec3 cameraPosition = markerPosition + glm::vec3(2.5f, 2.0f, 2.5f);
        return glm::lookAt(cameraPosition, markerPosition, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    if (m_PlanetFocusActive)
    {
        const std::vector<Planet>& planets = m_SolarSystem->GetPlanets();

        if (m_SelectedPlanetIndex >= 0 && m_SelectedPlanetIndex < static_cast<int>(planets.size()))
        {
            const Planet& planet = planets[m_SelectedPlanetIndex];
            glm::vec3 planetPosition = planet.GetPosition(elapsedDays);
            float effectiveRadius = ComputeEffectiveRadius(planet);
            float distance = effectiveRadius * 6.0f + 3.0f;

            glm::vec3 cameraPosition = planetPosition + glm::vec3(distance, distance * 0.4f, distance);
            return glm::lookAt(cameraPosition, planetPosition, glm::vec3(0.0f, 1.0f, 0.0f));
        }
    }

    return m_Camera->GetViewMatrix();
}

void Application::DrawPlanetWithRing(const Planet& planet, float elapsedDays, size_t planetIndex)
{
    float effectiveRadius = ComputeEffectiveRadius(planet);
    unsigned int textureId = (planetIndex < m_PlanetTextures.size() && m_PlanetTextures[planetIndex]->IsValid()) ? m_PlanetTextures[planetIndex]->GetId() : 0;

    m_Renderer->DrawSphere(planet.GetModelMatrix(elapsedDays, effectiveRadius), planet.GetColor(), planet.IsSun(), textureId);

    if (planet.GetName() == "Saturn")
    {
        glm::vec3 planetPosition = planet.GetPosition(elapsedDays);

        glm::mat4 ringModel(1.0f);
        ringModel = glm::translate(ringModel, planetPosition);
        ringModel = glm::rotate(ringModel, glm::radians(26.7f), glm::vec3(0.0f, 0.0f, 1.0f));
        ringModel = glm::scale(ringModel, glm::vec3(effectiveRadius * 1.6f));

        unsigned int ringTextureId = (m_SaturnRingTexture && m_SaturnRingTexture->IsValid()) ? m_SaturnRingTexture->GetId() : 0;
        m_Renderer->DrawRing(ringModel, ringTextureId);
    }
}

void Application::RenderAsteroidMarkers(float elapsedDays, double currentJulianDate)
{
    const std::vector<AsteroidRecord>& visible = m_AsteroidListState.GetVisibleAsteroids();

    for (int i = 0; i < static_cast<int>(visible.size()); ++i)
    {
        const AsteroidRecord& asteroid = visible[i];
        bool isSelected = m_AsteroidListState.HasSelection() && i == m_AsteroidListState.GetSelectedIndex();

        glm::vec3 markerPosition;

        if (asteroid.hasOrbitalElements)
        {
            markerPosition = KeplerOrbitCalculator::CalculateHeliocentricPosition(asteroid.orbitalElements, currentJulianDate, AuToUnitsScale);

            glm::vec3 lineColor = isSelected ? glm::vec3(1.0f, 0.85f, 0.2f) : glm::vec3(0.5f, 0.5f, 0.55f);
            m_Renderer->DrawDynamicLineLoop(asteroid.orbitPathPoints, lineColor);
        }
        else
        {
            const Planet* targetPlanet = FindPlanetByName(asteroid.targetBody);

            if (!targetPlanet)
            {
                continue;
            }

            glm::vec3 planetPosition = targetPlanet->GetPosition(elapsedDays);
            float overviewOffset = ComputeEffectiveRadius(*targetPlanet) * 3.0f;
            markerPosition = AsteroidPositioner::CalculateMarkerPosition(planetPosition, asteroid.designation, overviewOffset);
        }

        glm::vec3 color = isSelected ? glm::vec3(1.0f, 0.5f, 0.05f) : glm::vec3(0.9f, 0.9f, 0.9f);
        float radius = isSelected ? 0.35f : 0.18f;

        glm::mat4 model(1.0f);
        model = glm::translate(model, markerPosition);
        model = glm::scale(model, glm::vec3(radius));

        m_Renderer->DrawSphere(model, color, true, 0);
    }
}

void Application::RenderUI()
{
    m_ImGuiLayer->BeginFrame();

    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.82f);
    ImGui::Begin("Mission Control", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text(m_Simulation->IsPaused() ? "Simulation is paused" : "Simulation is running");
    ImGui::Text("Time speed: %.2fx", m_Simulation->GetSpeedMultiplier());
    ImGui::Text("Orbit paths: %s", m_ShowOrbitLines ? "Shown" : "Hidden");
    ImGui::Text("Planet sizes: %s", m_TrueScaleMode ? "True astronomical scale" : "Easy to see");
    ImGui::Separator();

    if (m_AsteroidLoadInProgress)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Loading live asteroid data...");
    }
    else if (!m_AsteroidsLoaded)
    {
        ImGui::TextDisabled("Press T to view live asteroids");

        if (!m_CursorLocked)
        {
            ImGui::SameLine();
            if (ImGui::Button("View Asteroids"))
            {
                EnableAsteroids();
            }
        }
    }
    else
    {
        ImGui::Text(m_AsteroidsEnabled ? "Asteroid tracking is on" : "Asteroid tracking is off");

        if (!m_CursorLocked)
        {
            if (ImGui::Button(m_AsteroidsEnabled ? "Hide Asteroids" : "Show Asteroids"))
            {
                m_AsteroidsEnabled = !m_AsteroidsEnabled;
            }
        }
    }

    ImGui::End();

    if (m_AsteroidsEnabled && !m_PlanetFocusActive)
    {
        float panelWidth = 380.0f;
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - panelWidth - 20.0f, 20.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panelWidth, 0.0f), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.82f);
        ImGui::Begin("Asteroid Watch", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::Text("Filter by planet:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "%s", m_AsteroidListState.GetCurrentFilter().c_str());
        ImGui::TextDisabled("Press Tab to switch planets");
        ImGui::Separator();

        const std::vector<AsteroidRecord>& visible = m_AsteroidListState.GetVisibleAsteroids();

        if (visible.empty())
        {
            ImGui::TextDisabled("No asteroids are currently approaching this planet");
        }
        else
        {
            for (int i = 0; i < static_cast<int>(visible.size()); ++i)
            {
                bool isSelected = (i == m_AsteroidListState.GetSelectedIndex());

                if (isSelected)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.05f, 1.0f));
                }

                std::string orbitLabel = visible[i].hasOrbitalElements ? "real orbit" : "approximate";
                ImGui::BulletText("%s (%s)", visible[i].designation.c_str(), orbitLabel.c_str());

                if (isSelected)
                {
                    ImGui::PopStyleColor();
                }
            }
        }

        ImGui::End();

        if (m_AsteroidListState.HasSelection())
        {
            const AsteroidRecord& selected = m_AsteroidListState.GetSelected();

            ImGui::SetNextWindowPos(ImVec2(20.0f, 200.0f), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.82f);
            ImGui::Begin("Selected Asteroid", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("Name: %s", selected.designation.c_str());
            ImGui::Text("Heading toward: %s", selected.targetBody.c_str());
            ImGui::Text("Closest approach: %s", selected.closeApproachDate.c_str());
            ImGui::Text("Distance at closest: %.5f AU", selected.distanceAu);
            ImGui::Text("Speed: %.2f km per second", selected.relativeVelocityKmS);
            ImGui::Spacing();
            ImGui::TextDisabled(m_FocusMode ? "Press Backspace to return to free flight" : "Press Enter to fly to this asteroid");
            ImGui::End();
        }
    }

    if (m_PlanetFocusActive)
    {
        const std::vector<Planet>& planets = m_SolarSystem->GetPlanets();

        if (m_SelectedPlanetIndex >= 0 && m_SelectedPlanetIndex < static_cast<int>(planets.size()))
        {
            const Planet& planet = planets[m_SelectedPlanetIndex];

            ImGui::SetNextWindowPos(ImVec2(20.0f, 200.0f), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.82f);
            ImGui::Begin("Selected Planet", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("Name: %s", planet.GetName().c_str());
            ImGui::Text("Distance from Sun: %.2f units", planet.GetDistanceFromSun());
            ImGui::Text("Orbital period: %.1f days", planet.GetOrbitalPeriod());
            ImGui::Spacing();
            ImGui::TextDisabled("Press Backspace to return to the full solar system");
            ImGui::End();
        }
    }

    ImGui::SetNextWindowPos(ImVec2(20.0f, io.DisplaySize.y - 360.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.78f);
    ImGui::Begin("How to Play", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "MOVE AROUND");
    ImGui::Text("WASD always moves the camera");
    ImGui::Text("Mouse looks around, Shift sprints, scroll zooms");

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "USING BUTTONS");
    ImGui::Text("Left Alt frees your mouse to click things, press it again to look around");

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "TIME");
    ImGui::Text("Space to pause, + and - to change speed, R to reset");

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "VIEW OPTIONS");
    ImGui::Text("O to show or hide orbit paths");
    ImGui::Text("V to switch between easy-to-see and true scale sizes");
    ImGui::Text("Number keys 1 to 8 to zoom in on one planet");
    ImGui::Text("Backspace to return to the full solar system");

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "ASTEROIDS");
    ImGui::Text("T to turn asteroid tracking on or off");
    ImGui::Text("Up and Down to browse, Tab to switch planets");
    ImGui::Text("Enter to fly closer, Backspace to fly back");

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "SAVING");
    ImGui::Text("K to save your view, L to load it back");

    ImGui::Spacing();
    ImGui::Text("Escape to quit");

    ImGui::End();

    m_ImGuiLayer->EndFrame();
}

void Application::ProcessFrame()
{
    float currentTime = static_cast<float>(glfwGetTime());
    float deltaTime = currentTime - m_LastFrameTime;
    m_LastFrameTime = currentTime;

    PollAsteroidLoad();

    if (glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(m_Window, true);
    }

    if (!m_FocusMode && !m_PlanetFocusActive)
    {
        m_Camera->ProcessKeyboard(m_Window, deltaTime);
    }

    m_Simulation->Update(deltaTime);

    float elapsedDays = m_Simulation->GetElapsedDays();
    double currentJulianDate = KeplerOrbitCalculator::GetCurrentJulianDate(static_cast<double>(elapsedDays));

    glm::mat4 projection = glm::perspective(
        glm::radians(m_Camera->GetFieldOfView()),
        static_cast<float>(m_ViewportWidth) / static_cast<float>(m_ViewportHeight),
        0.05f,
        6500.0f
    );

    glm::mat4 view = GetActiveViewMatrix(elapsedDays, currentJulianDate);
    m_Renderer->SetViewProjection(view, projection);
    m_Renderer->SetCameraPosition(m_Camera->GetPosition());

    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_Renderer->DrawStars();

    const std::vector<Planet>& planets = m_SolarSystem->GetPlanets();

    if (m_PlanetFocusActive && m_SelectedPlanetIndex >= 0 && m_SelectedPlanetIndex < static_cast<int>(planets.size()))
    {
        DrawPlanetWithRing(planets[m_SelectedPlanetIndex], elapsedDays, static_cast<size_t>(m_SelectedPlanetIndex));
    }
    else
    {
        for (const Planet& planet : planets)
        {
            if (m_ShowOrbitLines && planet.GetDistanceFromSun() > 0.0f)
            {
                m_Renderer->DrawOrbitLine(planet.GetOrbitModelMatrix(), OrbitLineColor);
            }
        }

        for (size_t i = 0; i < planets.size(); ++i)
        {
            DrawPlanetWithRing(planets[i], elapsedDays, i);
        }

        if (m_AsteroidsEnabled)
        {
            RenderAsteroidMarkers(elapsedDays, currentJulianDate);
        }
    }

    RenderUI();

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
    if (m_AsteroidLoadThread.joinable())
    {
        m_AsteroidLoadThread.join();
    }

    m_SaturnRingTexture.reset();
    m_PlanetTextures.clear();
    m_ImGuiLayer.reset();
    m_MongoRepository.reset();
    m_MongoEnvironment.reset();
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

void Application::PrintControls()
{
    std::cout << "OrbitScope controls" << std::endl;
    std::cout << "W A S D always moves the camera" << std::endl;
    std::cout << "Mouse looks around, only while the cursor is locked" << std::endl;
    std::cout << "Left Shift sprints while moving" << std::endl;
    std::cout << "Scroll wheel zooms" << std::endl;
    std::cout << "Left Alt frees the mouse cursor so you can click on-screen buttons" << std::endl;
    std::cout << "Space pauses or resumes the simulation" << std::endl;
    std::cout << "Plus increases simulation speed" << std::endl;
    std::cout << "Minus decreases simulation speed" << std::endl;
    std::cout << "R resets the simulation" << std::endl;
    std::cout << "O toggles orbit lines" << std::endl;
    std::cout << "V toggles between easy-to-see planet sizes and true astronomical scale" << std::endl;
    std::cout << "1 through 8 zooms the camera onto that single planet" << std::endl;
    std::cout << "Backspace returns to the full solar system view" << std::endl;
    std::cout << "K saves the current configuration" << std::endl;
    std::cout << "L loads the saved configuration" << std::endl;
    std::cout << "T turns live asteroid tracking on or off" << std::endl;
    std::cout << "Up and Down arrows move the asteroid list selection" << std::endl;
    std::cout << "Tab cycles the asteroid planet filter" << std::endl;
    std::cout << "Enter focuses the camera on the selected asteroid" << std::endl;
    std::cout << "H prints this control list again" << std::endl;
    std::cout << "Escape closes the application" << std::endl;
}

void Application::SaveCurrentConfig()
{
    if (!m_MongoRepository)
    {
        std::cout << "MongoDB not connected, cannot save configuration" << std::endl;
        return;
    }

    const std::vector<Planet>& planets = m_SolarSystem->GetPlanets();
    std::string selectedName;

    if (m_SelectedPlanetIndex >= 0 && m_SelectedPlanetIndex < static_cast<int>(planets.size()))
    {
        selectedName = planets[m_SelectedPlanetIndex].GetName();
    }

    SimulationConfig config;
    config.name = "quicksave";
    config.speed = m_Simulation->GetSpeedMultiplier();
    config.orbitLinesEnabled = m_ShowOrbitLines;
    config.trueScaleMode = m_TrueScaleMode;
    config.selectedPlanet = selectedName;

    ConfigRepository configRepository(*m_MongoRepository);

    if (configRepository.SaveConfig(config))
    {
        std::cout << "Saved simulation configuration" << std::endl;
    }
    else
    {
        std::cout << "Failed to save simulation configuration" << std::endl;
    }
}

void Application::LoadNamedConfig()
{
    if (!m_MongoRepository)
    {
        std::cout << "MongoDB not connected, cannot load configuration" << std::endl;
        return;
    }

    ConfigRepository configRepository(*m_MongoRepository);
    SimulationConfig config;

    if (!configRepository.LoadConfig("quicksave", config))
    {
        std::cout << "No saved configuration found" << std::endl;
        return;
    }

    m_Simulation->SetSpeedMultiplier(config.speed);
    m_ShowOrbitLines = config.orbitLinesEnabled;
    m_TrueScaleMode = config.trueScaleMode;

    const std::vector<Planet>& planets = m_SolarSystem->GetPlanets();
    for (int i = 0; i < static_cast<int>(planets.size()); ++i)
    {
        if (planets[i].GetName() == config.selectedPlanet)
        {
            m_SelectedPlanetIndex = i;
            break;
        }
    }

    std::cout << "Loaded simulation configuration" << std::endl;
}

void Application::MouseCallback(GLFWwindow* window, double xPos, double yPos)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

    if (!app->m_CursorLocked)
    {
        return;
    }

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
        app->m_TrueScaleMode = !app->m_TrueScaleMode;
    }
    else if (key == GLFW_KEY_K)
    {
        app->SaveCurrentConfig();
    }
    else if (key == GLFW_KEY_L)
    {
        app->LoadNamedConfig();
    }
    else if (key == GLFW_KEY_H)
    {
        app->PrintControls();
    }
    else if (key == GLFW_KEY_T)
    {
        if (app->m_AsteroidsLoaded)
        {
            app->m_AsteroidsEnabled = !app->m_AsteroidsEnabled;
        }
        else
        {
            app->EnableAsteroids();
        }
    }
    else if (key == GLFW_KEY_LEFT_ALT)
    {
        app->m_CursorLocked = !app->m_CursorLocked;
        glfwSetInputMode(window, GLFW_CURSOR, app->m_CursorLocked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        app->m_FirstMouse = true;
    }
    else if (key == GLFW_KEY_UP)
    {
        app->m_AsteroidListState.MoveSelectionUp();
    }
    else if (key == GLFW_KEY_DOWN)
    {
        app->m_AsteroidListState.MoveSelectionDown();
    }
    else if (key == GLFW_KEY_TAB)
    {
        app->m_AsteroidListState.CycleFilter();
    }
    else if (key == GLFW_KEY_ENTER)
    {
        if (app->m_AsteroidListState.HasSelection())
        {
            app->m_FocusMode = true;
        }
    }
    else if (key == GLFW_KEY_BACKSPACE)
    {
        app->m_FocusMode = false;
        app->m_PlanetFocusActive = false;
    }
    else if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
    {
        int index = key - GLFW_KEY_0;
        const std::vector<Planet>& planets = app->m_SolarSystem->GetPlanets();

        if (index >= 0 && index < static_cast<int>(planets.size()))
        {
            if (planets[index].IsSun())
            {
                std::cout << "The Sun cannot be focused on" << std::endl;
            }
            else
            {
                app->m_SelectedPlanetIndex = index;
                app->m_PlanetFocusActive = true;
                app->m_FocusMode = false;
                app->PrintSelectedPlanetInfo();
            }
        }
    }
}