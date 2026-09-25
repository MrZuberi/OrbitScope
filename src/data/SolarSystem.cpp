// Contains source code for the SolarSystem.cpp file
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
        { "Sun", 2.5f, 0.0f, 0.0f, 1.0f, 0.8f, 0.2f },
        { "Mercury", 0.2f, 4.0f, 88.0f, 0.6f, 0.6f, 0.6f },
        { "Venus", 0.35f, 5.5f, 224.7f, 0.9f, 0.7f, 0.4f },
        { "Earth", 0.4f, 7.0f, 365.25f, 0.2f, 0.5f, 0.9f },
        { "Mars", 0.3f, 8.5f, 687.0f, 0.8f, 0.3f, 0.2f },
        { "Jupiter", 1.2f, 11.0f, 4331.0f, 0.8f, 0.6f, 0.4f },
        { "Saturn", 1.0f, 14.0f, 10747.0f, 0.9f, 0.8f, 0.6f },
        { "Uranus", 0.7f, 17.0f, 30589.0f, 0.5f, 0.8f, 0.9f },
        { "Neptune", 0.65f, 20.0f, 59800.0f, 0.3f, 0.4f, 0.9f }
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
        m_Planets.emplace_back(record.name, record.radius, record.distanceFromSun, record.orbitalPeriod, glm::vec3(record.colorR, record.colorG, record.colorB));
    }
}