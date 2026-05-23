#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// POD cu stats pt fiecare tip de tower. Citit din data/towers.json.
// Cmpurile irelevante pt un tower raman 0 / vector gol (ex. damage = 0 pt Honeypot)
struct TowerSpec {
    std::string display_name;
    int   cost              = 0;
    float range             = 0.0f;

    // Antivirus / Adblocker
    float damage            = 0.0f;
    float attack_speed      = 0.0f;
    float projectile_speed  = 0.0f;

    // Honeypot
    float slow_factor       = 1.0f;   // 1.0 = no slow

    // Firewall
    float max_hp            = 0.0f;
    float regen_rate        = 0.0f;
    float block_radius      = 0.0f;

    // Miner
    int   income_per_wave   = 0;

    // common
    bool  requires_path     = false;
    std::vector<std::string> supports_abilities;
};

// nlohmann/json ADL, automat cand faci j.get<TowerSpec>().
inline void from_json(const nlohmann::json& j, TowerSpec& s) {
    j.at("display_name").get_to(s.display_name);
    j.at("cost").get_to(s.cost);
    j.at("range").get_to(s.range);
    j.at("requires_path").get_to(s.requires_path);
    j.at("supports_abilities").get_to(s.supports_abilities);

    //foloseste value() ca default
    s.damage           = j.value("damage", 0.0f);
    s.attack_speed     = j.value("attack_speed", 0.0f);
    s.projectile_speed = j.value("projectile_speed", 0.0f);
    s.slow_factor      = j.value("slow_factor", 1.0f);
    s.max_hp           = j.value("max_hp", 0.0f);
    s.regen_rate       = j.value("regen_rate", 0.0f);
    s.block_radius     = j.value("block_radius", 0.0f);
    s.income_per_wave  = j.value("income_per_wave", 0);
}
