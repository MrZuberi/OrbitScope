#include "models/Planet.h"

#include <glm/gtc/matrix_transform.hpp>

#include "simulation/OrbitCalculator.h"

Planet::Planet(const std::string& name, float radius, float distanceFromSun, float orbitalPeriod, const glm::vec3& color)
    : m_Name(name)
    , m_Radius(radius)
    , m_DistanceFromSun(distanceFromSun)
    , m_OrbitalPeriod(orbitalPeriod)
    , m_Color(color)
{
}

const std::string& Planet::GetName() const
{
    return m_Name;
}

float Planet::GetRadius() const
{
    return m_Radius;
}

float Planet::GetDistanceFromSun() const
{
    return m_DistanceFromSun;
}

float Planet::GetOrbitalPeriod() const
{
    return m_OrbitalPeriod;
}

const glm::vec3& Planet::GetColor() const
{
    return m_Color;
}

glm::mat4 Planet::GetModelMatrix(float elapsedDays, float radiusScale) const
{
    glm::vec3 position = OrbitCalculator::CalculatePosition(m_DistanceFromSun, m_OrbitalPeriod, elapsedDays);

    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, glm::vec3(m_Radius * radiusScale));
    return model;
}

glm::mat4 Planet::GetOrbitModelMatrix() const
{
    return glm::scale(glm::mat4(1.0f), glm::vec3(m_DistanceFromSun));
}

glm::vec3 Planet::GetPosition(float elapsedDays) const
{
    return OrbitCalculator::CalculatePosition(m_DistanceFromSun, m_OrbitalPeriod, elapsedDays);
}