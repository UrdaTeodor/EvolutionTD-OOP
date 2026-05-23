#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// wave-rile sunt fie intro hardcodate fie in functie de bugetul directorului
struct WaveSpec {
    std::vector<std::string> intro;          // enemy keys spawned hardcodat la start
    std::vector<std::string> unlocks_after;  // enemy keys deblocate pentru Director 
    int   base_budget    = 0;                // budget Director acest wave (0 = fara spawnuri random)
    bool  offers_major   = false;            // dupa wave: shop arata slot Major
    bool  is_boss        = false;            // wave de boss (uptiune extra in shop pentru urmatoarea portiune de joc, un fel de hardcore mode la alegere)
};

inline void from_json(const nlohmann::json& j, WaveSpec& s) {
    s.intro          = j.value("intro",          std::vector<std::string>{});
    s.unlocks_after  = j.value("unlocks_after",  std::vector<std::string>{});
    s.base_budget    = j.value("base_budget",    0);
    s.offers_major   = j.value("offers_major",   false);
    s.is_boss        = j.value("is_boss",        false);
}
