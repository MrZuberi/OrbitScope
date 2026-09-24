#include "simulation/KeplerOrbitCalculator.h"

#include <cmath>
#include <ctime>

namespace
{
    const double Pi = 3.14159265358979323846;
}

double KeplerOrbitCalculator::SolveEccentricAnomaly(double meanAnomalyRad, double eccentricity)
{
    double eccentricAnomaly = meanAnomalyRad;

    for (int i = 0; i < 50; ++i)
    {
        double delta = eccentricAnomaly - eccentricity * sin(eccentricAnomaly) - meanAnomalyRad;
        double derivative = 1.0 - eccentricity * cos(eccentricAnomaly);
        eccentricAnomaly -= delta / derivative;
    }

    return eccentricAnomaly;
}

glm::vec3 KeplerOrbitCalculator::PerifocalToWorld(double xPerifocal, double yPerifocal, const OrbitalElements& elements, float unitsPerAu)
{
    double inclinationRad = elements.inclinationDeg * Pi / 180.0;
    double argumentPeriapsisRad = elements.argumentPeriapsisDeg * Pi / 180.0;
    double ascendingNodeRad = elements.longitudeAscendingNodeDeg * Pi / 180.0;

    double x1 = xPerifocal * cos(argumentPeriapsisRad) - yPerifocal * sin(argumentPeriapsisRad);
    double y1 = xPerifocal * sin(argumentPeriapsisRad) + yPerifocal * cos(argumentPeriapsisRad);

    double x2 = x1;
    double y2 = y1 * cos(inclinationRad);
    double z2 = y1 * sin(inclinationRad);

    double xRef = x2 * cos(ascendingNodeRad) - y2 * sin(ascendingNodeRad);
    double yRef = x2 * sin(ascendingNodeRad) + y2 * cos(ascendingNodeRad);
    double zRef = z2;

    glm::vec3 world;
    world.x = static_cast<float>(xRef) * unitsPerAu;
    world.y = static_cast<float>(zRef) * unitsPerAu;
    world.z = static_cast<float>(yRef) * unitsPerAu;

    return world;
}

glm::vec3 KeplerOrbitCalculator::CalculateHeliocentricPosition(const OrbitalElements& elements, double currentJulianDate, float unitsPerAu)
{
    double daysSinceEpoch = currentJulianDate - elements.epochJulianDate;
    double meanMotionDegPerDay = 360.0 / elements.orbitalPeriodDays;

    double meanAnomalyDeg = elements.meanAnomalyDegAtEpoch + meanMotionDegPerDay * daysSinceEpoch;
    meanAnomalyDeg = fmod(meanAnomalyDeg, 360.0);
    if (meanAnomalyDeg < 0.0)
    {
        meanAnomalyDeg += 360.0;
    }

    double meanAnomalyRad = meanAnomalyDeg * Pi / 180.0;
    double eccentricAnomaly = SolveEccentricAnomaly(meanAnomalyRad, elements.eccentricity);

    double trueAnomaly = 2.0 * atan2(
        sqrt(1.0 + elements.eccentricity) * sin(eccentricAnomaly / 2.0),
        sqrt(1.0 - elements.eccentricity) * cos(eccentricAnomaly / 2.0)
    );

    double radius = elements.semiMajorAxisAu * (1.0 - elements.eccentricity * cos(eccentricAnomaly));

    double xPerifocal = radius * cos(trueAnomaly);
    double yPerifocal = radius * sin(trueAnomaly);

    return PerifocalToWorld(xPerifocal, yPerifocal, elements, unitsPerAu);
}

std::vector<glm::vec3> KeplerOrbitCalculator::BuildOrbitPath(const OrbitalElements& elements, float unitsPerAu, int segments)
{
    std::vector<glm::vec3> points;

    for (int i = 0; i < segments; ++i)
    {
        double trueAnomaly = 2.0 * Pi * (static_cast<double>(i) / static_cast<double>(segments));
        double radius = elements.semiMajorAxisAu * (1.0 - elements.eccentricity * elements.eccentricity) / (1.0 + elements.eccentricity * cos(trueAnomaly));

        double xPerifocal = radius * cos(trueAnomaly);
        double yPerifocal = radius * sin(trueAnomaly);

        points.push_back(PerifocalToWorld(xPerifocal, yPerifocal, elements, unitsPerAu));
    }

    return points;
}

double KeplerOrbitCalculator::GetCurrentJulianDate(double simulationElapsedDays)
{
    std::time_t now = std::time(nullptr);
    double unixJulianDate = static_cast<double>(now) / 86400.0 + 2440587.5;
    return unixJulianDate + simulationElapsedDays;
}