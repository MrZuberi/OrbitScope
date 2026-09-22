#include "data/PlanetRepository.h"

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

namespace
{
    const std::string PlanetsCollection = "planets";
}

PlanetRepository::PlanetRepository(MongoRepository& mongoRepository)
    : m_MongoRepository(mongoRepository)
{
}

bool PlanetRepository::LoadPlanets(std::vector<PlanetRecord>& outPlanets)
{
    std::vector<bsoncxx::document::value> documents;
    if (!m_MongoRepository.FindMany(PlanetsCollection, documents))
    {
        return false;
    }

    outPlanets.clear();

    for (const bsoncxx::document::value& document : documents)
    {
        bsoncxx::document::view view = document.view();

        PlanetRecord record;
        record.name = std::string(view["name"].get_string().value);
        record.radius = static_cast<float>(view["radius"].get_double().value);
        record.distanceFromSun = static_cast<float>(view["distanceFromSun"].get_double().value);
        record.orbitalPeriod = static_cast<float>(view["orbitalPeriod"].get_double().value);
        record.colorR = static_cast<float>(view["colorR"].get_double().value);
        record.colorG = static_cast<float>(view["colorG"].get_double().value);
        record.colorB = static_cast<float>(view["colorB"].get_double().value);

        outPlanets.push_back(record);
    }

    return !outPlanets.empty();
}

bool PlanetRepository::SeedPlanets(const std::vector<PlanetRecord>& planets)
{
    std::vector<bsoncxx::document::value> documents;

    for (const PlanetRecord& record : planets)
    {
        documents.push_back(make_document(
            kvp("name", record.name),
            kvp("radius", static_cast<double>(record.radius)),
            kvp("distanceFromSun", static_cast<double>(record.distanceFromSun)),
            kvp("orbitalPeriod", static_cast<double>(record.orbitalPeriod)),
            kvp("colorR", static_cast<double>(record.colorR)),
            kvp("colorG", static_cast<double>(record.colorG)),
            kvp("colorB", static_cast<double>(record.colorB))
        ));
    }

    return m_MongoRepository.InsertMany(PlanetsCollection, documents);
}