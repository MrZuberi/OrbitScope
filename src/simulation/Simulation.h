#pragma once

class Simulation
{
public:
    Simulation();

    void Update(float deltaTime);
    void TogglePause();
    void IncreaseSpeed();
    void DecreaseSpeed();
    void SetSpeedMultiplier(float speed);
    void Reset();

    float GetElapsedDays() const;
    bool IsPaused() const;
    float GetSpeedMultiplier() const;

private:
    float m_ElapsedDays;
    float m_SpeedMultiplier;
    bool m_Paused;
};