// Declares the SimulationConfig struct and related functions
#pragma once

#include <string>

struct SimulationConfig
{
    std::string name;
    float speed;
    bool orbitLinesEnabled;
    std::string selectedPlanet;
};