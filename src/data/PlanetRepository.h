#pragma once

#include <vector>

#include "data/MongoRepository.h"
#include "models/PlanetRecord.h"

class PlanetRepository
{
public:
    explicit PlanetRepository(MongoRepository& mongoRepository);

    bool LoadPlanets(std::vector<PlanetRecord>& outPlanets);
    bool SeedPlanets(const std::vector<PlanetRecord>& planets);

private:
    MongoRepository& m_MongoRepository;
};