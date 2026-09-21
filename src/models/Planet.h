#pragma once

#include <string>

#include <glm/glm.hpp>

class Planet
{
public:
    Planet(const std::string& name, float radius, float distanceFromSun, float orbitalPeriod, const glm::vec3& color);

    const std::string& GetName() const;
    float GetRadius() const;
    float GetDistanceFromSun() const;
    float GetOrbitalPeriod() const;
    const glm::vec3& GetColor() const;

    glm::mat4 GetModelMatrix(float elapsedDays) const;

private:
    std::string m_Name;
    float m_Radius;
    float m_DistanceFromSun;
    float m_OrbitalPeriod;
    glm::vec3 m_Color;
};