// Implements the AsteroidPositioner class representing an asteroid with orbital elements
#pragma once

#include <string>

#include <glm/glm.hpp>

class AsteroidPositioner
{
public:
    static glm::vec3 CalculateMarkerPosition(const glm::vec3& planetPosition, const std::string& designation, float markerOffset);
    static float CalculateCloseUpOffset(double distanceAu);
};