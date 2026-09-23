#pragma once

#include <string>

struct AsteroidRecord
{
    std::string designation;
    std::string targetBody;
    std::string closeApproachDate;
    double distanceAu;
    double relativeVelocityKmS;
    double absoluteMagnitude;
};