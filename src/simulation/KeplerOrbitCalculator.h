#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "models/AsteroidOrbitalElements.h"

class KeplerOrbitCalculator
{
public:
    static glm::vec3 CalculateHeliocentricPosition(const OrbitalElements& elements, double currentJulianDate, float unitsPerAu);
    static std::vector<glm::vec3> BuildOrbitPath(const OrbitalElements& elements, float unitsPerAu, int segments);
    static double GetCurrentJulianDate(double simulationElapsedDays);

private:
    static double SolveEccentricAnomaly(double meanAnomalyRad, double eccentricity);
    static glm::vec3 PerifocalToWorld(double xPerifocal, double yPerifocal, const OrbitalElements& elements, float unitsPerAu);
};