// Contains source code for the ConfigRepository.cpp file
#include "data/ConfigRepository.h"

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>

#include <vector>

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

namespace
{
    const std::string ConfigsCollection = "configs";
}

ConfigRepository::ConfigRepository(MongoRepository& mongoRepository)
    : m_MongoRepository(mongoRepository)
{
}

bool ConfigRepository::SaveConfig(const SimulationConfig& config)
{
    std::vector<bsoncxx::document::value> documents;

    documents.push_back(make_document(
        kvp("name", config.name),
        kvp("speed", static_cast<double>(config.speed)),
        kvp("orbitLinesEnabled", config.orbitLinesEnabled),
        kvp("selectedPlanet", config.selectedPlanet)
    ));

    return m_MongoRepository.InsertMany(ConfigsCollection, documents);
}

bool ConfigRepository::LoadConfig(const std::string& name, SimulationConfig& outConfig)
{
    std::vector<bsoncxx::document::value> documents;
    if (!m_MongoRepository.FindMany(ConfigsCollection, documents))
    {
        return false;
    }

    for (const bsoncxx::document::value& document : documents)
    {
        bsoncxx::document::view view = document.view();

        try
        {
            std::string documentName = std::string(view["name"].get_string().value);

            if (documentName != name)
            {
                continue;
            }

            outConfig.name = documentName;
            outConfig.speed = static_cast<float>(view["speed"].get_double().value);
            outConfig.orbitLinesEnabled = view["orbitLinesEnabled"].get_bool().value;
            outConfig.selectedPlanet = std::string(view["selectedPlanet"].get_string().value);

            return true;
        }
        catch (...)
        {
            continue;
        }
    }

    return false;
}