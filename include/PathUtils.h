#pragma once
#include <algorithm>
#include <utility>
#include <vector>

// Expandeaza waypoint-urile drumului (din MapSpec) in lista completa de celule
// (row, col). Partajat de BoardRenderer, FirewallTower (sarja ShieldedRunner)
// si balance_sim.
inline std::vector<std::pair<int, int>> expandPathCells(
        const std::vector<std::pair<int, int>>& waypoints) {
    std::vector<std::pair<int, int>> cells;
    for (size_t i = 0; i + 1 < waypoints.size(); i++) {
        int r1 = waypoints[i].first,     c1 = waypoints[i].second;
        int r2 = waypoints[i + 1].first, c2 = waypoints[i + 1].second;
        if (r1 == r2) {
            for (int c = std::min(c1, c2); c <= std::max(c1, c2); c++)
                cells.push_back({r1, c});
        } else {
            for (int r = std::min(r1, r2); r <= std::max(r1, r2); r++)
                cells.push_back({r, c1});
        }
    }
    return cells;
}
