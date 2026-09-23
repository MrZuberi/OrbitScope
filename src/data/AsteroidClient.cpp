#include "data/AsteroidClient.h"
#include "data/Http.h"

#include <nlohmann/json.hpp>

#include <ctime>
#include <unordered_map>

namespace
{
    std::string FormatDate(std::time_t time)
    {
        std::tm* tmPtr = std::gmtime(&time);
        char buffer[16];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", tmPtr);
        return std::string(buffer);
    }
}

bool AsteroidClient::FetchCloseApproaches(std::vector<AsteroidRecord>& outAsteroids, double maxDistanceAu, int lookAheadDays)
{
    std::time_t now = std::time(nullptr);
    std::time_t future = now + static_cast<std::time_t>(lookAheadDays) * 86400;

    std::string dateMin = FormatDate(now);
    std::string dateMax = FormatDate(future);

    std::string url = "https://ssd-api.jpl.nasa.gov/cad.api?dist-max=" + std::to_string(maxDistanceAu)
        + "&body=ALL&date-min=" + dateMin + "&date-max=" + dateMax + "&sort=dist";

    std::string response;
    if (!Http::Get(url, {}, response))
    {
        return false;
    }

    try
    {
        nlohmann::json parsed = nlohmann::json::parse(response);

        if (!parsed.contains("fields") || !parsed.contains("data"))
        {
            return false;
        }

        std::unordered_map<std::string, size_t> fieldIndex;
        const auto& fields = parsed["fields"];
        for (size_t i = 0; i < fields.size(); ++i)
        {
            fieldIndex[fields[i].get<std::string>()] = i;
        }

        outAsteroids.clear();

        for (const auto& row : parsed["data"])
        {
            AsteroidRecord record;
            record.designation = row[fieldIndex["des"]].is_null() ? std::string() : row[fieldIndex["des"]].get<std::string>();
            record.targetBody = row[fieldIndex["body"]].is_null() ? std::string() : row[fieldIndex["body"]].get<std::string>();
            record.closeApproachDate = row[fieldIndex["cd"]].is_null() ? std::string() : row[fieldIndex["cd"]].get<std::string>();

            std::string distanceString = row[fieldIndex["dist"]].is_null() ? "0" : row[fieldIndex["dist"]].get<std::string>();
            std::string velocityString = row[fieldIndex["v_rel"]].is_null() ? "0" : row[fieldIndex["v_rel"]].get<std::string>();
            std::string magnitudeString = row[fieldIndex["h"]].is_null() ? "0" : row[fieldIndex["h"]].get<std::string>();

            record.distanceAu = std::stod(distanceString);
            record.relativeVelocityKmS = std::stod(velocityString);
            record.absoluteMagnitude = std::stod(magnitudeString);

            outAsteroids.push_back(record);
        }

        return true;
    }
    catch (...)
    {
        return false;
    }
}