#include "simulation/AsteroidPositioner.h"

#include <functional>
#include <cmath>

namespace
{
    const float Pi = 3.14159265358979323846f;
    const float CloseUpBaseOffset = 3.0f;
    const float CloseUpScaleFactor = 400.0f;
}

glm::vec3 AsteroidPositioner::CalculateMarkerPosition(const glm::vec3& planetPosition, const std::string& designation, float markerOffset)
{
    size_t hashValue = std::hash<std::string>{}(designation);

    float azimuth = static_cast<float>(hashValue % 3600) / 3600.0f * 2.0f * Pi;
    float elevation = static_cast<float>((hashValue / 3600) % 1800) / 1800.0f * Pi - (Pi / 2.0f);

    glm::vec3 offset;
    offset.x = markerOffset * cosf(elevation) * cosf(azimuth);
    offset.y = markerOffset * sinf(elevation);
    offset.z = markerOffset * cosf(elevation) * sinf(azimuth);

    return planetPosition + offset;
}

float AsteroidPositioner::CalculateCloseUpOffset(double distanceAu)
{
    return CloseUpBaseOffset + static_cast<float>(distanceAu) * CloseUpScaleFactor;
}