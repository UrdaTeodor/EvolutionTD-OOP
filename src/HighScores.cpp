#include "HighScores.h"
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

HighScores::HighScores() {
    load();
}

void HighScores::load() {
    std::ifstream in(HIGH_SCORES_PATH);
    if (!in) return;   // fisier inexistent

    nlohmann::json j;
    try {
        in >> j;
    } catch (const nlohmann::json::parse_error&) {
        return;   // malformat
    }

    for (auto it = j.begin(); it != j.end(); ++it) {
        MapStats s;
        s.best_wave_reached  = it.value().value("best_wave_reached",  0);
        s.total_runs         = it.value().value("total_runs",         0);
        s.total_kills        = it.value().value("total_kills",        0);
        s.total_money_earned = it.value().value("total_money_earned", 0);
        stats_[it.key()] = s;
    }
}

void HighScores::save() const {
    nlohmann::json j = nlohmann::json::object();
    for (const auto& [map_id, s] : stats_) {
        j[map_id] = {
            {"best_wave_reached",  s.best_wave_reached},
            {"total_runs",         s.total_runs},
            {"total_kills",        s.total_kills},
            {"total_money_earned", s.total_money_earned},
        };
    }
    std::ofstream out(HIGH_SCORES_PATH);
    if (!out) return;   
    out << j.dump(2);
}

void HighScores::recordRun(const std::string& map_id,
                           int wave_reached, int kills, int money_earned) {
    MapStats& s = stats_[map_id];
    s.best_wave_reached    = std::max(s.best_wave_reached, wave_reached);
    s.total_runs          += 1;
    s.total_kills         += kills;
    s.total_money_earned  += money_earned;
    save();
}

HighScores::MapStats HighScores::getStats(const std::string& map_id) const {
    auto it = stats_.find(map_id);
    if (it == stats_.end()) return {};
    return it->second;
}
