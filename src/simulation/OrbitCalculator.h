// Implements the OrbitCalculator class that calculates orbital positions using Kepler's equation
#pragma once

#include <glm/glm.hpp>

class OrbitCalculator
{
public:
    static glm::vec3 CalculatePosition(float distanceFromSun, float orbitalPeriodDays, float elapsedDays);
};
