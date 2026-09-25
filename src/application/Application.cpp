// Contains functions for loading planetary textures and managing rendering resources
#include "application/Application.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include "data/PlanetRepository.h"
#include "data/AsteroidClient.h"
#include "data/SBDBClient.h"
#include "simulation/AsteroidPositioner.h"
#include "simulation/KeplerOrbitCalculator.h"

#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <vector>

namespace
{
    const glm::vec3 OrbitLineColor(0.35f, 0.35f, 0.4f);
    const double AsteroidMaxDistanceAu = 0.05;
    const int AsteroidLookAheadDays = 60;
    const float AuToUnitsScale = 110.0f;
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
    , m_AsteroidsEnabled(false)
    , m_AsteroidsLoaded(false)
    , m_ViewingAsteroid(false)
    , m_NeedsRecenter(false)
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
    m_Camera = std::make_unique<Camera>(glm::vec3(0.0f, 70.0f, 230.0f));
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
    m_AsteroidTexture = std::make_unique<Texture>("resources/textures/asteroid.jpg");
}

void Application::EnableAsteroids()
{
    if (m_AsteroidsLoaded)
    {
        m_AsteroidsEnabled = true;
        m_ViewingAsteroid = false;
        m_NeedsRecenter = true;
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
    m_ViewingAsteroid = false;
    m_NeedsRecenter = true;
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

void Application::RecenterCamera(float elapsedDays, double currentJulianDate)
{
    if (m_ViewingAsteroid && m_AsteroidListState.HasSelection())
    {
        const AsteroidRecord& selected = m_AsteroidListState.GetSelected();
        glm::vec3 target;
        bool valid = false;

        if (selected.hasOrbitalElements)
        {
            target = KeplerOrbitCalculator::CalculateHeliocentricPosition(selected.orbitalElements, currentJulianDate, AuToUnitsScale);
            valid = true;
        }
        else
        {
            const Planet* targetPlanet = FindPlanetByName(selected.targetBody);

            if (targetPlanet)
            {
                glm::vec3 planetPosition = targetPlanet->GetRealHeliocentricPosition(elapsedDays, AuToUnitsScale);
                float closeUpOffset = AsteroidPositioner::CalculateCloseUpOffset(selected.distanceAu);
                target = AsteroidPositioner::CalculateMarkerPosition(planetPosition, selected.designation, closeUpOffset);
                valid = true;
            }
        }

        if (valid)
        {
            glm::vec3 cameraPosition = target + glm::vec3(2.5f, 2.0f, 2.5f);
            m_Camera->SetPositionAndTarget(cameraPosition, target);
        }
    }
    else
    {
        const Planet* planet = FindPlanetByName(m_AsteroidListState.GetCurrentFilter());

        if (planet)
        {
            glm::vec3 target = planet->GetRealHeliocentricPosition(elapsedDays, AuToUnitsScale);
            float distance = planet->GetRadius() * 6.0f + 4.0f;
            glm::vec3 cameraPosition = target + glm::vec3(distance, distance * 0.4f, distance);
            m_Camera->SetPositionAndTarget(cameraPosition, target);
        }
    }
}

void Application::DrawPlanetWithRing(const Planet& planet, const glm::vec3& position, float elapsedDays, size_t planetIndex)
{
    float effectiveRadius = planet.GetRadius();
    unsigned int textureId = (planetIndex < m_PlanetTextures.size() && m_PlanetTextures[planetIndex]->IsValid()) ? m_PlanetTextures[planetIndex]->GetId() : 0;

    m_Renderer->DrawSphere(planet.GetModelMatrixAtPosition(position, elapsedDays, effectiveRadius), planet.GetColor(), planet.IsSun(), textureId);

    if (planet.GetName() == "Saturn")
    {
        glm::mat4 ringModel(1.0f);
        ringModel = glm::translate(ringModel, position);
        ringModel = glm::rotate(ringModel, glm::radians(26.7f), glm::vec3(0.0f, 0.0f, 1.0f));
        ringModel = glm::scale(ringModel, glm::vec3(effectiveRadius * 1.6f));

        unsigned int ringTextureId = (m_SaturnRingTexture && m_SaturnRingTexture->IsValid()) ? m_SaturnRingTexture->GetId() : 0;
        m_Renderer->DrawRing(ringModel, ringTextureId);
    }
}

void Application::RenderAsteroidMarkers(float elapsedDays, double currentJulianDate)
{
    const std::vector<AsteroidRecord>& visible = m_AsteroidListState.GetVisibleAsteroids();
    unsigned int asteroidTextureId = (m_AsteroidTexture && m_AsteroidTexture->IsValid()) ? m_AsteroidTexture->GetId() : 0;

    for (int i = 0; i < static_cast<int>(visible.size()); ++i)
    {
        bool isSelected = m_AsteroidListState.HasSelection() && i == m_AsteroidListState.GetSelectedIndex();

        if (m_ViewingAsteroid && !isSelected)
        {
            continue;
        }

        const AsteroidRecord& asteroid = visible[i];
        glm::vec3 markerPosition;

        if (asteroid.hasOrbitalElements)
        {
            markerPosition = KeplerOrbitCalculator::CalculateHeliocentricPosition(asteroid.orbitalElements, currentJulianDate, AuToUnitsScale);

            if (m_ShowOrbitLines)
            {
                glm::vec3 lineColor = isSelected ? glm::vec3(1.0f, 0.85f, 0.2f) : glm::vec3(0.5f, 0.5f, 0.55f);
                m_Renderer->DrawDynamicLineLoop(asteroid.orbitPathPoints, lineColor);
            }
        }
        else
        {
            const Planet* targetPlanet = FindPlanetByName(asteroid.targetBody);

            if (!targetPlanet)
            {
                continue;
            }

            glm::vec3 planetPosition = targetPlanet->GetRealHeliocentricPosition(elapsedDays, AuToUnitsScale);
            float overviewOffset = targetPlanet->GetRadius() * 3.0f;
            markerPosition = AsteroidPositioner::CalculateMarkerPosition(planetPosition, asteroid.designation, overviewOffset);
        }

        glm::vec3 color = isSelected ? glm::vec3(1.0f, 0.5f, 0.05f) : glm::vec3(0.85f, 0.85f, 0.85f);
        float radius = isSelected ? 0.35f : 0.18f;
        int variantIndex = static_cast<int>(std::hash<std::string>{}(asteroid.designation) % 6);

        glm::mat4 model(1.0f);
        model = glm::translate(model, markerPosition);
        model = glm::scale(model, glm::vec3(radius));

        m_Renderer->DrawAsteroid(model, color, asteroidTextureId, variantIndex);
    }
}

void Application::RenderUI(const glm::mat4& view, const glm::mat4& projection, float elapsedDays)
{
    m_ImGuiLayer->BeginFrame();

    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Always, ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.82f);
    ImGui::Begin("Mission Control", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text(m_Simulation->IsPaused() ? "Simulation is paused" : "Simulation is running");
    ImGui::Text("Time speed: %.2fx", m_Simulation->GetSpeedMultiplier());
    ImGui::Text("Orbit paths: %s", m_ShowOrbitLines ? "Shown" : "Hidden");
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
        ImGui::Text(m_AsteroidsEnabled ? "Asteroid mode is on" : "Asteroid mode is off");

        if (!m_CursorLocked)
        {
            if (ImGui::Button(m_AsteroidsEnabled ? "Exit Asteroid Mode" : "Enter Asteroid Mode"))
            {
                if (m_AsteroidsEnabled)
                {
                    m_AsteroidsEnabled = false;
                }
                else
                {
                    EnableAsteroids();
                }
            }
        }
    }

    ImGui::End();

    if (m_AsteroidsEnabled)
    {
        float panelWidth = 380.0f;
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 20.0f, 20.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::SetNextWindowSizeConstraints(ImVec2(panelWidth, 0.0f), ImVec2(panelWidth, io.DisplaySize.y - 240.0f));
        ImGui::SetNextWindowBgAlpha(0.82f);
        ImGui::Begin("Asteroid Watch", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::Text("Now viewing:");
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

        ImGui::SetNextWindowPos(ImVec2(20.0f, 190.0f), ImGuiCond_Always, ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.82f);
        ImGui::Begin("Currently Viewing", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

        if (m_ViewingAsteroid && m_AsteroidListState.HasSelection())
        {
            const AsteroidRecord& selected = m_AsteroidListState.GetSelected();
            ImGui::Text("Asteroid: %s", selected.designation.c_str());
            ImGui::Text("Heading toward: %s", selected.targetBody.c_str());
            ImGui::Text("Closest approach: %s", selected.closeApproachDate.c_str());
            ImGui::Text("Distance at closest: %.5f AU", selected.distanceAu);
            ImGui::Text("Speed: %.2f km per second", selected.relativeVelocityKmS);
        }
        else
        {
            const Planet* planet = FindPlanetByName(m_AsteroidListState.GetCurrentFilter());

            if (planet)
            {
                ImGui::Text("Planet: %s", planet->GetName().c_str());
                ImGui::TextDisabled("Use Up and Down to pick an asteroid to view");
            }
        }

        ImGui::End();

        if (m_ViewingAsteroid)
        {
            const Planet* planet = FindPlanetByName(m_AsteroidListState.GetCurrentFilter());

            if (planet)
            {
                glm::vec3 planetPosition = planet->GetRealHeliocentricPosition(elapsedDays, AuToUnitsScale);
                glm::vec4 clip = projection * view * glm::vec4(planetPosition, 1.0f);

                if (clip.w < 0.0f)
                {
                    clip.x = -clip.x;
                    clip.y = -clip.y;
                }

                float absW = fabsf(clip.w) > 0.0001f ? fabsf(clip.w) : 0.0001f;
                float ndcX = clip.x / absW;
                float ndcY = clip.y / absW;

                bool onScreen = clip.w > 0.0f && ndcX > -0.92f && ndcX < 0.92f && ndcY > -0.92f && ndcY < 0.92f;

                if (!onScreen)
                {
                    float screenCenterX = io.DisplaySize.x * 0.5f;
                    float screenCenterY = io.DisplaySize.y * 0.5f;

                    float dirX = ndcX;
                    float dirY = -ndcY;
                    float length = sqrtf(dirX * dirX + dirY * dirY);
                    if (length < 0.0001f)
                    {
                        length = 0.0001f;
                    }
                    dirX /= length;
                    dirY /= length;

                    float edgeRadius = (io.DisplaySize.y < io.DisplaySize.x ? io.DisplaySize.y : io.DisplaySize.x) * 0.42f;
                    float arrowX = screenCenterX + dirX * edgeRadius;
                    float arrowY = screenCenterY + dirY * edgeRadius;

                    float angle = atan2f(dirY, dirX);
                    float arrowSize = 22.0f;

                    ImVec2 tip(arrowX + cosf(angle) * arrowSize, arrowY + sinf(angle) * arrowSize);
                    ImVec2 left(arrowX + cosf(angle + 2.6f) * arrowSize, arrowY + sinf(angle + 2.6f) * arrowSize);
                    ImVec2 right(arrowX + cosf(angle - 2.6f) * arrowSize, arrowY + sinf(angle - 2.6f) * arrowSize);

                    ImDrawList* drawList = ImGui::GetForegroundDrawList();
                    drawList->AddTriangleFilled(tip, left, right, IM_COL32(255, 210, 60, 255));
                    drawList->AddText(ImVec2(arrowX - 30.0f, arrowY + 24.0f), IM_COL32(255, 210, 60, 255), planet->GetName().c_str());
                }
            }
        }
    }

    ImGui::SetNextWindowPos(ImVec2(20.0f, io.DisplaySize.y - 20.0f), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
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
    ImGui::Text("O to show or hide orbit paths");

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "ASTEROID MODE");
    ImGui::Text("T to enter or exit asteroid mode");
    ImGui::Text("Tab to switch which planet you are viewing");
    ImGui::Text("Up and Down to pick an asteroid to fly to");

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

    m_Camera->ProcessKeyboard(m_Window, deltaTime);
    m_Simulation->Update(deltaTime);

    float elapsedDays = m_Simulation->GetElapsedDays();
    double currentJulianDate = KeplerOrbitCalculator::GetCurrentJulianDate(static_cast<double>(elapsedDays));

    if (m_AsteroidsEnabled && m_NeedsRecenter)
    {
        RecenterCamera(elapsedDays, currentJulianDate);
        m_NeedsRecenter = false;
    }

    glm::mat4 projection = glm::perspective(
        glm::radians(m_Camera->GetFieldOfView()),
        static_cast<float>(m_ViewportWidth) / static_cast<float>(m_ViewportHeight),
        0.05f,
        6500.0f
    );

    glm::mat4 view = m_Camera->GetViewMatrix();
    m_Renderer->SetViewProjection(view, projection);
    m_Renderer->SetCameraPosition(m_Camera->GetPosition());

    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_Renderer->DrawStars();

    const std::vector<Planet>& planets = m_SolarSystem->GetPlanets();

    if (m_AsteroidsEnabled)
    {
        for (size_t i = 0; i < planets.size(); ++i)
        {
            if (planets[i].GetName() == m_AsteroidListState.GetCurrentFilter())
            {
                glm::vec3 realPosition = planets[i].GetRealHeliocentricPosition(elapsedDays, AuToUnitsScale);
                DrawPlanetWithRing(planets[i], realPosition, elapsedDays, i);
                break;
            }
        }

        RenderAsteroidMarkers(elapsedDays, currentJulianDate);
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
            DrawPlanetWithRing(planets[i], planets[i].GetPosition(elapsedDays), elapsedDays, i);
        }
    }

    RenderUI(view, projection, elapsedDays);

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

    m_AsteroidTexture.reset();
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

void Application::PrintControls()
{
    std::cout << "OrbitScope controls" << std::endl;
    std::cout << "The app opens showing all planets and their orbit lines" << std::endl;
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
    std::cout << "T enters or exits asteroid mode, showing one planet and its live asteroids" << std::endl;
    std::cout << "Tab, while in asteroid mode, switches which planet you are viewing" << std::endl;
    std::cout << "Up and Down arrows pick an asteroid, which then centers the camera on it" << std::endl;
    std::cout << "H prints this control list again" << std::endl;
    std::cout << "Escape closes the application" << std::endl;
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
    else if (key == GLFW_KEY_H)
    {
        app->PrintControls();
    }
    else if (key == GLFW_KEY_T)
    {
        if (app->m_AsteroidsEnabled)
        {
            app->m_AsteroidsEnabled = false;
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
        if (app->m_AsteroidsEnabled)
        {
            app->m_AsteroidListState.MoveSelectionUp();
            app->m_ViewingAsteroid = true;
            app->m_NeedsRecenter = true;
        }
    }
    else if (key == GLFW_KEY_DOWN)
    {
        if (app->m_AsteroidsEnabled)
        {
            app->m_AsteroidListState.MoveSelectionDown();
            app->m_ViewingAsteroid = true;
            app->m_NeedsRecenter = true;
        }
    }
    else if (key == GLFW_KEY_TAB)
    {
        if (app->m_AsteroidsEnabled)
        {
            app->m_AsteroidListState.CycleFilter();
            app->m_ViewingAsteroid = false;
            app->m_NeedsRecenter = true;
        }
    }
}