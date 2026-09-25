// Contains source code for the AsteroidListState.cpp file
#include "simulation/AsteroidListState.h"
#include "simulation/AsteroidFilter.h"

AsteroidListState::AsteroidListState()
    : m_FilterIndex(0)
    , m_SelectedIndex(-1)
{
}

void AsteroidListState::SetSource(const std::vector<AsteroidRecord>& asteroids, const std::vector<std::string>& planetNames)
{
    m_AllAsteroids = asteroids;
    m_FilterOptions = planetNames;
    m_FilterIndex = 0;
    Rebuild();
}

void AsteroidListState::Rebuild()
{
    if (m_FilterOptions.empty())
    {
        m_VisibleAsteroids.clear();
        m_SelectedIndex = -1;
        return;
    }

    m_VisibleAsteroids = AsteroidFilter::ByPlanet(m_AllAsteroids, m_FilterOptions[m_FilterIndex]);
    m_SelectedIndex = m_VisibleAsteroids.empty() ? -1 : 0;
}

void AsteroidListState::MoveSelectionUp()
{
    if (m_VisibleAsteroids.empty())
    {
        return;
    }

    int count = static_cast<int>(m_VisibleAsteroids.size());
    m_SelectedIndex = (m_SelectedIndex - 1 + count) % count;
}

void AsteroidListState::MoveSelectionDown()
{
    if (m_VisibleAsteroids.empty())
    {
        return;
    }

    int count = static_cast<int>(m_VisibleAsteroids.size());
    m_SelectedIndex = (m_SelectedIndex + 1) % count;
}

void AsteroidListState::CycleFilter()
{
    if (m_FilterOptions.empty())
    {
        return;
    }

    m_FilterIndex = (m_FilterIndex + 1) % static_cast<int>(m_FilterOptions.size());
    Rebuild();
}

void AsteroidListState::SetFilterByName(const std::string& name)
{
    for (int i = 0; i < static_cast<int>(m_FilterOptions.size()); ++i)
    {
        if (m_FilterOptions[i] == name)
        {
            m_FilterIndex = i;
            Rebuild();
            return;
        }
    }
}

const std::vector<AsteroidRecord>& AsteroidListState::GetVisibleAsteroids() const
{
    return m_VisibleAsteroids;
}

int AsteroidListState::GetSelectedIndex() const
{
    return m_SelectedIndex;
}

bool AsteroidListState::HasSelection() const
{
    return m_SelectedIndex >= 0 && m_SelectedIndex < static_cast<int>(m_VisibleAsteroids.size());
}

const AsteroidRecord& AsteroidListState::GetSelected() const
{
    return m_VisibleAsteroids[m_SelectedIndex];
}

const std::string& AsteroidListState::GetCurrentFilter() const
{
    static const std::string empty;
    return m_FilterOptions.empty() ? empty : m_FilterOptions[m_FilterIndex];
}