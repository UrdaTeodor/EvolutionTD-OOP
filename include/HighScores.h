#pragma once
#include <string>
#include <unordered_map>

class HighScores {
public:
    struct MapStats {
        int best_wave_reached    = 0;
        int total_runs           = 0;
        int total_kills          = 0;
        int total_money_earned   = 0;
    };

    static constexpr const char* HIGH_SCORES_PATH = "data/high_scores.json";


    HighScores();
    void recordRun(const std::string& map_id,
                   int wave_reached, int kills, int money_earned);


    MapStats getStats(const std::string& map_id) const;

private:
    std::unordered_map<std::string, MapStats> stats_;

    void load();
    void save() const;
};
