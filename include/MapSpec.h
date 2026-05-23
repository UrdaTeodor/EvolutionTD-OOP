#pragma once
#include <string>
#include <vector>
#include <utility>
#include <nlohmann/json.hpp>

// Definitia unei harti. path = waypoints (row, col) pentru drum.
// wave_ids = ordinea wave-urilor (cheile din waves.json).
struct MapSpec {
    std::string name;
    int   grid_size   = 20;
    int   start_money = 100;
    int   start_hp    = 100;

    // waypoints: vector de (row, col). Drumul e segmente intre waypoints succesive.
    std::vector<std::pair<int, int>> path;

    std::vector<std::string> wave_ids;
};

inline void from_json(const nlohmann::json& j, MapSpec& s) {
    j.at("name").get_to(s.name);
    s.grid_size   = j.value("grid_size",   20);
    s.start_money = j.value("start_money", 100);
     s.start_hp    = j.value("start_hp",    100);

    s.path.clear();
    for (const auto& wp : j.at("path")) {
        // fiecare waypoint = [row, col]
        s.path.emplace_back(wp.at(0).get<int>(), wp.at(1).get<int>());
    }

    j.at("wave_ids").get_to(s.wave_ids);
}
