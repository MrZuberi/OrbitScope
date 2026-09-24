#pragma once

#include <string>

struct SimulationConfig
{
    std::string name;
    float speed;
    bool orbitLinesEnabled;
    bool trueScaleMode;
    std::string selectedPlanet;
};