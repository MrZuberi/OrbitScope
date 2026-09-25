// Contains source code for the AsteroidFilter.cpp file
#include "simulation/AsteroidFilter.h"

std::vector<AsteroidRecord> AsteroidFilter::ByPlanet(const std::vector<AsteroidRecord>& asteroids, const std::string& planetName)
{
    std::vector<AsteroidRecord> result;

    for (const AsteroidRecord& asteroid : asteroids)
    {
        if (asteroid.targetBody == planetName)
        {
            result.push_back(asteroid);
        }
    }

    return result;
}