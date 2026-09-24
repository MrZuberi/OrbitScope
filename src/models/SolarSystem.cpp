#include "models/SolarSystem.h"

SolarSystem::SolarSystem()
{
    BuildDefaultPlanets();
}

SolarSystem::SolarSystem(const std::vector<PlanetRecord>& records)
{
    BuildFromRecords(records);
}

const std::vector<Planet>& SolarSystem::GetPlanets() const
{
    return m_Planets;
}

std::vector<PlanetRecord> SolarSystem::GetDefaultRecords()
{
    return {
        { "Sun", 4.6520f, 0.0f, 0.0f, 0.0f, 1.0f, 0.8f, 0.2f, "resources/textures/sun.jpg", 609.12f },
        { "Mercury", 0.1915f, 14.0f, 0.387f, 88.0f, 0.6f, 0.6f, 0.6f, "resources/textures/mercury.jpg", 1407.6f },
        { "Venus", 0.4752f, 22.0f, 0.723f, 224.7f, 0.9f, 0.7f, 0.4f, "resources/textures/venus.jpg", -5832.5f },
        { "Earth", 0.5000f, 30.0f, 1.000f, 365.25f, 0.2f, 0.5f, 0.9f, "resources/textures/earth.jpg", 23.93f },
        { "Mars", 0.2661f, 40.0f, 1.524f, 687.0f, 0.8f, 0.3f, 0.2f, "resources/textures/mars.jpg", 24.62f },
        { "Jupiter", 5.4870f, 65.0f, 5.203f, 4331.0f, 0.8f, 0.6f, 0.4f, "resources/textures/jupiter.jpg", 9.93f },
        { "Saturn", 4.5720f, 95.0f, 9.537f, 10747.0f, 0.9f, 0.8f, 0.6f, "resources/textures/saturn.jpg", 10.66f },
        { "Uranus", 1.9910f, 130.0f, 19.191f, 30589.0f, 0.5f, 0.8f, 0.9f, "resources/textures/uranus.jpg", -17.24f },
        { "Neptune", 1.9330f, 165.0f, 30.069f, 59800.0f, 0.3f, 0.4f, 0.9f, "resources/textures/neptune.jpg", 16.11f }
    };
}

void SolarSystem::BuildDefaultPlanets()
{
    BuildFromRecords(GetDefaultRecords());
}

void SolarSystem::BuildFromRecords(const std::vector<PlanetRecord>& records)
{
    for (const PlanetRecord& record : records)
    {
        m_Planets.emplace_back(record.name, record.radius, record.distanceFromSun, record.realDistanceAu, record.orbitalPeriod, glm::vec3(record.colorR, record.colorG, record.colorB), record.texturePath, record.rotationPeriodHours);
    }
}