#include "data/SBDBClient.h"
#include "data/Http.h"

#include <nlohmann/json.hpp>

#include <unordered_map>

namespace
{
    std::string UrlEncodeSpaces(const std::string& input)
    {
        std::string result;

        for (char character : input)
        {
            if (character == ' ')
            {
                result += "%20";
            }
            else
            {
                result += character;
            }
        }

        return result;
    }
}

bool SBDBClient::FetchOrbitalElements(const std::string& designation, OrbitalElements& outElements)
{
    std::string url = "https://ssd-api.jpl.nasa.gov/sbdb.api?sstr=" + UrlEncodeSpaces(designation) + "&full-prec=true";

    std::string response;
    if (!Http::Get(url, {}, response))
    {
        return false;
    }

    try
    {
        nlohmann::json parsed = nlohmann::json::parse(response);

        if (!parsed.contains("orbit") || !parsed["orbit"].contains("elements"))
        {
            return false;
        }

        std::unordered_map<std::string, double> elementValues;

        for (const auto& element : parsed["orbit"]["elements"])
        {
            std::string name = element.value("name", std::string());
            std::string valueString = element.value("value", std::string());

            if (!name.empty() && !valueString.empty())
            {
                elementValues[name] = std::stod(valueString);
            }
        }

        if (elementValues.find("e") == elementValues.end() || elementValues.find("a") == elementValues.end())
        {
            return false;
        }

        outElements.eccentricity = elementValues["e"];
        outElements.semiMajorAxisAu = elementValues["a"];
        outElements.inclinationDeg = elementValues.count("i") ? elementValues["i"] : 0.0;
        outElements.longitudeAscendingNodeDeg = elementValues.count("om") ? elementValues["om"] : 0.0;
        outElements.argumentPeriapsisDeg = elementValues.count("w") ? elementValues["w"] : 0.0;
        outElements.meanAnomalyDegAtEpoch = elementValues.count("ma") ? elementValues["ma"] : 0.0;

        std::string epochString = parsed["orbit"].value("epoch", std::string("2451545.0"));
        outElements.epochJulianDate = std::stod(epochString);

        outElements.orbitalPeriodDays = 365.25 * std::pow(outElements.semiMajorAxisAu, 1.5);

        return true;
    }
    catch (...)
    {
        return false;
    }
}