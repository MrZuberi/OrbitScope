#include "simulation/AsteroidListState.h"
#include "simulation/AsteroidFilter.h"

AsteroidListState::AsteroidListState()
    : m_FilterIndex(0)
    , m_SelectedIndex(-1)
{
    m_FilterOptions.push_back("All");
}

void AsteroidListState::SetSource(const std::vector<AsteroidRecord>& asteroids, const std::vector<std::string>& planetNames)
{
    m_AllAsteroids = asteroids;

    m_FilterOptions.clear();
    m_FilterOptions.push_back("All");

    for (const std::string& name : planetNames)
    {
        m_FilterOptions.push_back(name);
    }

    m_FilterIndex = 0;
    Rebuild();
}

void AsteroidListState::Rebuild()
{
    const std::string& currentFilter = m_FilterOptions[m_FilterIndex];

    if (currentFilter == "All")
    {
        m_VisibleAsteroids = m_AllAsteroids;
    }
    else
    {
        m_VisibleAsteroids = AsteroidFilter::ByPlanet(m_AllAsteroids, currentFilter);
    }

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
    m_FilterIndex = (m_FilterIndex + 1) % static_cast<int>(m_FilterOptions.size());
    Rebuild();
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
    return m_FilterOptions[m_FilterIndex];
}