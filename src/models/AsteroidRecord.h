#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "models/AsteroidOrbitalElements.h"

struct AsteroidRecord
{
    std::string designation;
    std::string targetBody;
    std::string closeApproachDate;
    double distanceAu;
    double relativeVelocityKmS;
    double absoluteMagnitude;
    bool hasOrbitalElements = false;
    OrbitalElements orbitalElements{};
    std::vector<glm::vec3> orbitPathPoints;
};