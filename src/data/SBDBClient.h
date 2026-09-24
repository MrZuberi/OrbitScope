#pragma once

#include <string>

#include "models/AsteroidOrbitalElements.h"

class SBDBClient
{
public:
    bool FetchOrbitalElements(const std::string& designation, OrbitalElements& outElements);
};