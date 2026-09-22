#pragma once

#include <string>

class NASAClient
{
public:
    explicit NASAClient(std::string apiKey);

    bool FetchAstronomyPictureOfDay(std::string& outTitle, std::string& outExplanation);

private:
    std::string m_ApiKey;
};