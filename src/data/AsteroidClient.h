// Implements the AsteroidClient class representing an asteroid with orbital elements
#pragma once

#include <string>
#include <vector>

#include "models/AsteroidRecord.h"

class AsteroidClient
{
public:
    bool FetchCloseApproaches(std::vector<AsteroidRecord>& outAsteroids, double maxDistanceAu, int lookAheadDays);
};