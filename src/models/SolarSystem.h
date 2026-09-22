#pragma once

#include <vector>

#include "models/Planet.h"
#include "models/PlanetRecord.h"

class SolarSystem
{
public:
    SolarSystem();
    explicit SolarSystem(const std::vector<PlanetRecord>& records);

    const std::vector<Planet>& GetPlanets() const;

    static std::vector<PlanetRecord> GetDefaultRecords();

private:
    void BuildDefaultPlanets();
    void BuildFromRecords(const std::vector<PlanetRecord>& records);

    std::vector<Planet> m_Planets;
};