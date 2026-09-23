#include "application/Application.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "data/PlanetRepository.h"
#include "data/ConfigRepository.h"
#include "data/NASAClient.h"
#include "data/AsteroidClient.h"
#include "simulation/SimulationConfig.h"
#include "simulation/AsteroidPositioner.h"

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>

#include <cstdlib>
#include <iostream>
#include <vector>

namespace
{
    const glm::vec3 OrbitLineColor(0.4f, 0.4f, 0.4f);
    const float VisualScaleMultiplier = 3.0f;
    const double AsteroidMaxDistanceAu = 0.05;
    const int AsteroidLookAheadDays = 60;
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
    , m_FocusMode(false)
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
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_Renderer = std::make_unique<Renderer>();
    m_Camera = std::make_unique<Camera>(glm::vec3(0.0f, 12.0f, 28.0f));
    m_Simulation = std::make_unique<Simulation>();
    m_TextRenderer = std::make_unique<TextRenderer>("resources/fonts/arial.ttf", 22.0f);
    m_TextRenderer->SetScreenSize(m_ViewportWidth, m_ViewportHeight);

    const char* mongoUri = std::getenv("MONGODB_URI");
    const char* nasaApiKey = std::getenv("NASA_API_KEY");

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

    if (nasaApiKey)
    {
        NASAClient nasaClient(nasaApiKey);
        std::string title;
        std::string explanation;

        if (nasaClient.FetchAstronomyPictureOfDay(title, explanation))
        {
            std::cout << "NASA Astronomy Picture of the Day: " << title << std::endl;

            if (m_MongoRepository)
            {
                using bsoncxx::builder::basic::kvp;
                using bsoncxx::builder::basic::make_document;

                std::vector<bsoncxx::document::value> facts;
                facts.push_back(make_document(kvp("title", title), kvp("explanation", explanation)));
                m_MongoRepository->InsertMany("facts", facts);
            }
        }
        else
        {
            std::cout << "Could not reach NASA API, continuing without it" << std::endl;
        }
    }

    LoadAsteroidData();

    PrintControls();

    return true;
}

void Application::LoadAsteroidData()
{
    AsteroidClient asteroidClient;

    if (!asteroidClient.FetchCloseApproaches(m_Asteroids, AsteroidMaxDistanceAu, AsteroidLookAheadDays))
    {
        std::cout << "Could not fetch asteroid close approach data" << std::endl;
        return;
    }

    std::cout << "Fetched " << m_Asteroids.size() << " asteroid close approaches" << std::endl;

    std::vector<std::string> planetNames;
    for (const Planet& planet : m_SolarSystem->GetPlanets())
    {
        if (planet.GetDistanceFromSun() > 0.0f)
        {
            planetNames.push_back(planet.GetName());
        }
    }

    m_AsteroidListState.SetSource(m_Asteroids, planetNames);
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

glm::mat4 Application::GetActiveViewMatrix(float elapsedDays)
{
    if (!m_FocusMode || !m_AsteroidListState.HasSelection())
    {
        return m_Camera->GetViewMatrix();
    }

    const AsteroidRecord& selected = m_AsteroidListState.GetSelected();
    const Planet* targetPlanet = FindPlanetByName(selected.targetBody);

    if (!targetPlanet)
    {
        return m_Camera->GetViewMatrix();
    }

    glm::vec3 planetPosition = targetPlanet->GetPosition(elapsedDays);
    float closeUpOffset = AsteroidPositioner::CalculateCloseUpOffset(selected.distanceAu);
    glm::vec3 markerPosition = AsteroidPositioner::CalculateMarkerPosition(planetPosition, selected.designation, closeUpOffset);

    glm::vec3 direction = glm::normalize(markerPosition - planetPosition);
    glm::vec3 cameraPosition = markerPosition + direction * 4.0f + glm::vec3(0.0f, 2.0f, 0.0f);

    return glm::lookAt(cameraPosition, markerPosition, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Application::RenderAsteroidMarkers(float elapsedDays)
{
    const std::vector<AsteroidRecord>& visible = m_AsteroidListState.GetVisibleAsteroids();

    for (int i = 0; i < static_cast<int>(visible.size()); ++i)
    {
        const AsteroidRecord& asteroid = visible[i];
        const Planet* targetPlanet = FindPlanetByName(asteroid.targetBody);

        if (!targetPlanet)
        {
            continue;
        }

        glm::vec3 planetPosition = targetPlanet->GetPosition(elapsedDays);
        float overviewOffset = targetPlanet->GetRadius() * 3.0f;
        glm::vec3 markerPosition = AsteroidPositioner::CalculateMarkerPosition(planetPosition, asteroid.designation, overviewOffset);

        bool isSelected = m_AsteroidListState.HasSelection() && i == m_AsteroidListState.GetSelectedIndex();
        glm::vec3 color = isSelected ? glm::vec3(1.0f, 0.85f, 0.2f) : glm::vec3(0.9f, 0.9f, 0.9f);
        float radius = isSelected ? 0.09f : 0.05f;

        glm::mat4 model(1.0f);
        model = glm::translate(model, markerPosition);
        model = glm::scale(model, glm::vec3(radius));

        m_Renderer->DrawSphere(model, color, true);
    }
}

void Application::RenderUI()
{
    glDisable(GL_DEPTH_TEST);

    float panelWidth = 320.0f;
    float panelX = static_cast<float>(m_ViewportWidth) - panelWidth - 20.0f;
    float panelY = 20.0f;
    float lineHeight = 24.0f;

    const std::vector<AsteroidRecord>& visible = m_AsteroidListState.GetVisibleAsteroids();
    float panelHeight = lineHeight * (static_cast<float>(visible.size()) + 2.0f) + 20.0f;

    m_TextRenderer->RenderQuad(panelX, panelY, panelWidth, panelHeight, glm::vec3(0.0f, 0.0f, 0.0f), 0.55f);
    m_TextRenderer->RenderText("Filter: " + m_AsteroidListState.GetCurrentFilter(), panelX + 10.0f, panelY + lineHeight, glm::vec3(1.0f, 1.0f, 1.0f));

    for (size_t i = 0; i < visible.size(); ++i)
    {
        glm::vec3 color = (static_cast<int>(i) == m_AsteroidListState.GetSelectedIndex()) ? glm::vec3(1.0f, 0.85f, 0.2f) : glm::vec3(0.8f, 0.8f, 0.8f);
        std::string line = visible[i].designation + "  " + visible[i].targetBody;
        m_TextRenderer->RenderText(line, panelX + 10.0f, panelY + lineHeight * (static_cast<float>(i) + 2.0f), color);
    }

    if (m_AsteroidListState.HasSelection())
    {
        const AsteroidRecord& selected = m_AsteroidListState.GetSelected();

        m_TextRenderer->RenderQuad(20.0f, 20.0f, 380.0f, 140.0f, glm::vec3(0.0f, 0.0f, 0.0f), 0.55f);
        m_TextRenderer->RenderText("Asteroid " + selected.designation, 30.0f, 44.0f, glm::vec3(1.0f, 1.0f, 1.0f));
        m_TextRenderer->RenderText("Approaching " + selected.targetBody, 30.0f, 68.0f, glm::vec3(0.8f, 0.8f, 0.8f));
        m_TextRenderer->RenderText("Date " + selected.closeApproachDate, 30.0f, 92.0f, glm::vec3(0.8f, 0.8f, 0.8f));
        m_TextRenderer->RenderText("Distance " + std::to_string(selected.distanceAu) + " AU", 30.0f, 116.0f, glm::vec3(0.8f, 0.8f, 0.8f));
        m_TextRenderer->RenderText("Velocity " + std::to_string(selected.relativeVelocityKmS) + " km per second", 30.0f, 140.0f, glm::vec3(0.8f, 0.8f, 0.8f));
    }

    m_TextRenderer->RenderText("Tab changes filter, Up and Down select, Enter focuses, Backspace exits focus", 20.0f, static_cast<float>(m_ViewportHeight) - 20.0f, glm::vec3(0.6f, 0.6f, 0.6f));

    glEnable(GL_DEPTH_TEST);
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

    if (!m_FocusMode)
    {
        m_Camera->ProcessKeyboard(m_Window, deltaTime);
    }

    m_Simulation->Update(deltaTime);

    float elapsedDays = m_Simulation->GetElapsedDays();

    glm::mat4 projection = glm::perspective(
        glm::radians(m_Camera->GetFieldOfView()),
        static_cast<float>(m_ViewportWidth) / static_cast<float>(m_ViewportHeight),
        0.1f,
        300.0f
    );

    glm::mat4 view = GetActiveViewMatrix(elapsedDays);
    m_Renderer->SetViewProjection(view, projection);

    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_Renderer->DrawStars();

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
        bool isLightSource = (planet.GetDistanceFromSun() <= 0.0f);
        m_Renderer->DrawSphere(planet.GetModelMatrix(elapsedDays, radiusScale), planet.GetColor(), isLightSource);
    }

    RenderAsteroidMarkers(elapsedDays);
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
    m_TextRenderer.reset();
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
    std::cout << "W A S D moves the camera" << std::endl;
    std::cout << "Left Shift sprints while moving" << std::endl;
    std::cout << "Mouse looks around" << std::endl;
    std::cout << "Scroll wheel zooms" << std::endl;
    std::cout << "Space pauses or resumes the simulation" << std::endl;
    std::cout << "Plus increases simulation speed" << std::endl;
    std::cout << "Minus decreases simulation speed" << std::endl;
    std::cout << "R resets the simulation" << std::endl;
    std::cout << "O toggles orbit lines" << std::endl;
    std::cout << "V toggles visual scale mode" << std::endl;
    std::cout << "0 through 8 selects the Sun and each planet" << std::endl;
    std::cout << "K saves the current configuration" << std::endl;
    std::cout << "L loads the saved configuration" << std::endl;
    std::cout << "Up and Down arrows move the asteroid list selection" << std::endl;
    std::cout << "Tab cycles the asteroid planet filter" << std::endl;
    std::cout << "Enter focuses the camera on the selected asteroid" << std::endl;
    std::cout << "Backspace exits asteroid focus mode" << std::endl;
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
    config.visualScaleMode = m_VisualScaleMode;
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
    m_VisualScaleMode = config.visualScaleMode;

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

    if (app->m_TextRenderer)
    {
        app->m_TextRenderer->SetScreenSize(width, height);
    }
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
    }
    else if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
    {
        int index = key - GLFW_KEY_0;
        app->m_SelectedPlanetIndex = index;
        app->PrintSelectedPlanetInfo();
    }
}