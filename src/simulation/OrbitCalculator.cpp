// Contains source code for the OrbitCalculator.cpp file
#include "simulation/OrbitCalculator.h"

#include <cmath>

namespace
{
    const float TwoPi = 6.28318530717958647692f;
}

glm::vec3 OrbitCalculator::CalculatePosition(float distanceFromSun, float orbitalPeriodDays, float elapsedDays)
{
    if (orbitalPeriodDays <= 0.0f)
    {
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }

    float angle = TwoPi * (elapsedDays / orbitalPeriodDays);

    float x = distanceFromSun * cosf(angle);
    float z = distanceFromSun * sinf(angle);

    return glm::vec3(x, 0.0f, z);
}