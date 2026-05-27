#pragma once
#include <string>
#include <unordered_map>

// Buffuri globale pe tip de tower (cheia e string-ul din DataRegistry).
// Cand cumperi un Mini (ex.+5% damage Antivirus), se face buffsForType("antivirus").damage_pct += 0.05.
// Cand un tower calculeaza un stat efectiv, multiplica baza din spec cu (1 + pct). (deci nu exponential)
//
// stats relevante difera per tower type (Honeypot nu are damage, Firewall nu are atk speed).
struct TowerTypeBuffs {
    float damage_pct       = 0.0f;
    float range_pct        = 0.0f;
    float attack_speed_pct = 0.0f;
    float max_hp_pct       = 0.0f;   // Firewall
    float regen_pct        = 0.0f;   // Firewall
    float slow_pct         = 0.0f;   // Honeypot 
    float income_pct       = 0.0f;   // Miner
};

class GlobalStatBuffs {
    std::unordered_map<std::string, TowerTypeBuffs> per_type_;

public:
    // Lookup non-const
    // Daca cheia nu exista o creeaza cu valori 0.
    TowerTypeBuffs& mutable_for(const std::string& type_key) {
        return per_type_[type_key];
    }

    // Lookup const returneaza o copie (sau zero default daca tipul nu exista).
    TowerTypeBuffs for_type(const std::string& type_key) const {
        auto it = per_type_.find(type_key);
        if (it == per_type_.end()) return {};
        return it->second;
    }

    // Iterare pentru save/load.
    // cppcheck-suppress unusedFunction
    const std::unordered_map<std::string, TowerTypeBuffs>& all() const { return per_type_; }
};
