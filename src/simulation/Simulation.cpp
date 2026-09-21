#include "simulation/Simulation.h"

#include <algorithm>

namespace
{
    const float MinSpeed = 0.1f;
    const float MaxSpeed = 500.0f;
    const float SpeedStep = 1.5f;
    const float DaysPerSecondBase = 5.0f;
}

Simulation::Simulation()
    : m_ElapsedDays(0.0f)
    , m_SpeedMultiplier(1.0f)
    , m_Paused(false)
{
}

void Simulation::Update(float deltaTime)
{
    if (m_Paused)
    {
        return;
    }

    m_ElapsedDays += deltaTime * DaysPerSecondBase * m_SpeedMultiplier;
}

void Simulation::TogglePause()
{
    m_Paused = !m_Paused;
}

void Simulation::IncreaseSpeed()
{
    m_SpeedMultiplier = std::clamp(m_SpeedMultiplier * SpeedStep, MinSpeed, MaxSpeed);
}

void Simulation::DecreaseSpeed()
{
    m_SpeedMultiplier = std::clamp(m_SpeedMultiplier / SpeedStep, MinSpeed, MaxSpeed);
}

void Simulation::Reset()
{
    m_ElapsedDays = 0.0f;
    m_SpeedMultiplier = 1.0f;
    m_Paused = false;
}

float Simulation::GetElapsedDays() const
{
    return m_ElapsedDays;
}

bool Simulation::IsPaused() const
{
    return m_Paused;
}

float Simulation::GetSpeedMultiplier() const
{
    return m_SpeedMultiplier;
}