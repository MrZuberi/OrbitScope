#include "application/Application.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "models/Planet.h"
#include "data/PlanetRepository.h"
#include "data/ConfigRepository.h"
#include "data/NASAClient.h"
#include "data/AsteroidClient.h"
#include "simulation/SimulationConfig.h"
#include "simulation/AsteroidFilter.h"
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

    m_Renderer = std::make_unique<Renderer>();
    m_Camera = std::make_unique<Camera>(glm::vec3(0.0f, 12.0f, 28.0f));
    m_Simulation = std::make_unique<Simulation>();

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

    const std::vector<Planet>& planets = m_SolarSystem->GetPlanets();

    for (const Planet& planet : planets)
    {
        std::vector<AsteroidRecord> filtered = AsteroidFilter::ByPlanet(m_Asteroids, planet.GetName());

        if (filtered.empty())
        {
            continue;
        }

        std::cout << planet.GetName() << " has " << filtered.size() << " close approaching asteroids" << std::endl;

        glm::vec3 planetPosition = planet.GetPosition(m_Simulation->GetElapsedDays());

        for (const AsteroidRecord& asteroid : filtered)
        {
            glm::vec3 markerPosition = AsteroidPositioner::CalculateMarkerPosition(planetPosition, asteroid.designation, planet.GetRadius() * 3.0f);

            std::cout << "  " << asteroid.designation
                << " approaches on " << asteroid.closeApproachDate
                << " at " << asteroid.distanceAu << " AU"
                << " relative velocity " << asteroid.relativeVelocityKmS << " km per second"
                << " marker position " << markerPosition.x << " " << markerPosition.y << " " << markerPosition.z
                << std::endl;
        }
    }
}

void Application::UpdateProjection()
{
    glm::mat4 projection = glm::perspective(
        glm::radians(m_Camera->GetFieldOfView()),
        static_cast<float>(m_ViewportWidth) / static_cast<float>(m_ViewportHeight),
        0.1f,
        300.0f
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

    m_Renderer->DrawStars();

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
        bool isLightSource = (planet.GetDistanceFromSun() <= 0.0f);
        m_Renderer->DrawSphere(planet.GetModelMatrix(elapsedDays, radiusScale), planet.GetColor(), isLightSource);
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
    else if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
    {
        int index = key - GLFW_KEY_0;
        app->m_SelectedPlanetIndex = index;
        app->PrintSelectedPlanetInfo();
    }
}