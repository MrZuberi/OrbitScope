#pragma once

#include <string>

#include "data/MongoRepository.h"
#include "simulation/SimulationConfig.h"

class ConfigRepository
{
public:
    explicit ConfigRepository(MongoRepository& mongoRepository);

    bool SaveConfig(const SimulationConfig& config);
    bool LoadConfig(const std::string& name, SimulationConfig& outConfig);

private:
    MongoRepository& m_MongoRepository;
};