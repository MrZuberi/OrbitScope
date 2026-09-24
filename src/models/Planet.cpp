#include "models/Planet.h"

#include <glm/gtc/matrix_transform.hpp>

#include "simulation/OrbitCalculator.h"

namespace
{
    const float RotationVisualSlowdown = 60.0f;
}

Planet::Planet(const std::string& name, float radius, float distanceFromSun, float realDistanceAu, float orbitalPeriod, const glm::vec3& color, const std::string& texturePath, float rotationPeriodHours)
    : m_Name(name)
    , m_Radius(radius)
    , m_DistanceFromSun(distanceFromSun)
    , m_RealDistanceAu(realDistanceAu)
    , m_OrbitalPeriod(orbitalPeriod)
    , m_Color(color)
    , m_TexturePath(texturePath)
    , m_RotationPeriodHours(rotationPeriodHours)
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

const std::string& Planet::GetTexturePath() const
{
    return m_TexturePath;
}

bool Planet::IsSun() const
{
    return m_DistanceFromSun <= 0.0f;
}

glm::mat4 Planet::GetModelMatrixAtPosition(const glm::vec3& position, float elapsedDays, float effectiveRadius) const
{
    float rotationDegrees = m_RotationPeriodHours != 0.0f ? (elapsedDays * 24.0f / m_RotationPeriodHours) * 360.0f / RotationVisualSlowdown : 0.0f;

    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(effectiveRadius));
    return model;
}

glm::mat4 Planet::GetModelMatrix(float elapsedDays, float effectiveRadius) const
{
    glm::vec3 position = OrbitCalculator::CalculatePosition(m_DistanceFromSun, m_OrbitalPeriod, elapsedDays);
    return GetModelMatrixAtPosition(position, elapsedDays, effectiveRadius);
}

glm::mat4 Planet::GetOrbitModelMatrix() const
{
    return glm::scale(glm::mat4(1.0f), glm::vec3(m_DistanceFromSun));
}

glm::vec3 Planet::GetPosition(float elapsedDays) const
{
    return OrbitCalculator::CalculatePosition(m_DistanceFromSun, m_OrbitalPeriod, elapsedDays);
}

glm::vec3 Planet::GetRealHeliocentricPosition(float elapsedDays, float auToUnitsScale) const
{
    return OrbitCalculator::CalculatePosition(m_RealDistanceAu * auToUnitsScale, m_OrbitalPeriod, elapsedDays);
}