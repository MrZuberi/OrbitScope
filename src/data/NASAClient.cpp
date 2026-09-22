#include "data/NASAClient.h"
#include "data/Http.h"

#include <nlohmann/json.hpp>

NASAClient::NASAClient(std::string apiKey)
    : m_ApiKey(std::move(apiKey))
{
}

bool NASAClient::FetchAstronomyPictureOfDay(std::string& outTitle, std::string& outExplanation)
{
    std::string url = "https://api.nasa.gov/planetary/apod?api_key=" + m_ApiKey;

    std::string response;
    if (!Http::Get(url, {}, response))
    {
        return false;
    }

    try
    {
        nlohmann::json parsed = nlohmann::json::parse(response);
        outTitle = parsed.value("title", std::string());
        outExplanation = parsed.value("explanation", std::string());
        return true;
    }
    catch (...)
    {
        return false;
    }
}