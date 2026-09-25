// Implements the Planet class representing a celestial body with orbital properties
#pragma once

#include <string>

#include <glm/glm.hpp>

class Planet
{
public:
    Planet(const std::string& name, float radius, float distanceFromSun, float realDistanceAu, float orbitalPeriod, const glm::vec3& color, const std::string& texturePath, float rotationPeriodHours);

    const std::string& GetName() const;
    float GetRadius() const;
    float GetDistanceFromSun() const;
    float GetOrbitalPeriod() const;
    const glm::vec3& GetColor() const;
    const std::string& GetTexturePath() const;
    bool IsSun() const;

    glm::mat4 GetModelMatrix(float elapsedDays, float effectiveRadius) const;
    glm::mat4 GetModelMatrixAtPosition(const glm::vec3& position, float elapsedDays, float effectiveRadius) const;
    glm::mat4 GetOrbitModelMatrix() const;
    glm::vec3 GetPosition(float elapsedDays) const;
    glm::vec3 GetRealHeliocentricPosition(float elapsedDays, float auToUnitsScale) const;

private:
    std::string m_Name;
    float m_Radius;
    float m_DistanceFromSun;
    float m_RealDistanceAu;
    float m_OrbitalPeriod;
    glm::vec3 m_Color;
    std::string m_TexturePath;
    float m_RotationPeriodHours;
};