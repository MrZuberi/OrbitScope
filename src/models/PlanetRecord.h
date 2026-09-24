#pragma once

#include <string>

struct PlanetRecord
{
    std::string name;
    float radius;
    float distanceFromSun;
    float realDistanceAu;
    float orbitalPeriod;
    float colorR;
    float colorG;
    float colorB;
    std::string texturePath;
    float rotationPeriodHours;
};