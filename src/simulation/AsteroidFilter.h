#pragma once

#include <string>
#include <vector>

#include "models/AsteroidRecord.h"

class AsteroidFilter
{
public:
    static std::vector<AsteroidRecord> ByPlanet(const std::vector<AsteroidRecord>& asteroids, const std::string& planetName);
};