#include "models/Planet.h"

#include <glm/gtc/matrix_transform.hpp>

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

glm::mat4 Planet::GetModelMatrix() const
{
    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(m_DistanceFromSun, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(m_Radius));
    return model;
}