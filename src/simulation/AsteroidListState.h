#pragma once

#include <string>
#include <vector>

#include "models/AsteroidRecord.h"

class AsteroidListState
{
public:
    AsteroidListState();

    void SetSource(const std::vector<AsteroidRecord>& asteroids, const std::vector<std::string>& planetNames);
    void MoveSelectionUp();
    void MoveSelectionDown();
    void CycleFilter();

    const std::vector<AsteroidRecord>& GetVisibleAsteroids() const;
    int GetSelectedIndex() const;
    bool HasSelection() const;
    const AsteroidRecord& GetSelected() const;
    const std::string& GetCurrentFilter() const;

private:
    void Rebuild();

    std::vector<AsteroidRecord> m_AllAsteroids;
    std::vector<std::string> m_FilterOptions;
    std::vector<AsteroidRecord> m_VisibleAsteroids;
    int m_FilterIndex;
    int m_SelectedIndex;
};