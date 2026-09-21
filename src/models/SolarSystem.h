#pragma once

#include <vector>

#include "models/Planet.h"

class SolarSystem
{
public:
    SolarSystem();

    const std::vector<Planet>& GetPlanets() const;

private:
    void BuildDefaultPlanets();

    std::vector<Planet> m_Planets;
};