#pragma once
#include <string>
#include <nlohmann/json.hpp>

// Citit din data/enemies.json.
struct EnemySpec {
    std::string display_name;
    float max_health    = 0.0f;
    float speed         = 0.0f;
    int   reward        = 0;
    int   director_cost = 0;     // 0 = nu spawneaza Directorul (intro hardcodat/eventual alte runde hardcodate pe viitor)
    bool  is_boss       = false;
};

inline void from_json(const nlohmann::json& j, EnemySpec& s) {
    j.at("display_name").get_to(s.display_name);
    j.at("max_health").get_to(s.max_health);
    j.at("speed").get_to(s.speed);
    j.at("reward").get_to(s.reward);
    s.director_cost = j.value("director_cost", 0);
    s.is_boss       = j.value("is_boss", false);
}
