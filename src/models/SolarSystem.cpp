#include "models/SolarSystem.h"

SolarSystem::SolarSystem()
{
    BuildDefaultPlanets();
}

const std::vector<Planet>& SolarSystem::GetPlanets() const
{
    return m_Planets;
}

void SolarSystem::BuildDefaultPlanets()
{
    m_Planets.emplace_back("Sun", 2.5f, 0.0f, 0.0f, glm::vec3(1.0f, 0.8f, 0.2f));
    m_Planets.emplace_back("Mercury", 0.2f, 4.0f, 88.0f, glm::vec3(0.6f, 0.6f, 0.6f));
    m_Planets.emplace_back("Venus", 0.35f, 5.5f, 224.7f, glm::vec3(0.9f, 0.7f, 0.4f));
    m_Planets.emplace_back("Earth", 0.4f, 7.0f, 365.25f, glm::vec3(0.2f, 0.5f, 0.9f));
    m_Planets.emplace_back("Mars", 0.3f, 8.5f, 687.0f, glm::vec3(0.8f, 0.3f, 0.2f));
    m_Planets.emplace_back("Jupiter", 1.2f, 11.0f, 4331.0f, glm::vec3(0.8f, 0.6f, 0.4f));
    m_Planets.emplace_back("Saturn", 1.0f, 14.0f, 10747.0f, glm::vec3(0.9f, 0.8f, 0.6f));
    m_Planets.emplace_back("Uranus", 0.7f, 17.0f, 30589.0f, glm::vec3(0.5f, 0.8f, 0.9f));
    m_Planets.emplace_back("Neptune", 0.65f, 20.0f, 59800.0f, glm::vec3(0.3f, 0.4f, 0.9f));
}