#pragma once
#include <string>
#include <unordered_map>

// Buff-uri globale procentuale, indexate pe (tip de tower, nume de stat).
//
// Cheile de stat sunt EXACT stat_field-urile din data/evolutions.json
// ("damage_pct", "income_pct", "slow_pct", ...). Inainte exista un struct
// `TowerTypeBuffs` cu campuri fixe pentru toate stat-urile, ceea ce facea ca
// FIECARE tip de turn sa care si campuri care nu-l privesc (un Honeypot avea
// un income_pct mereu 0, un Miner un damage_pct mereu 0) si cerea atins in
// 5-6 locuri la fiecare stat nou (struct + dispatch in shop + save/load).
// Acum un (tip, stat) exista in harta DOAR daca a fost cumparat ceva, iar
// adaugarea unui stat nou e pur din date.
class GlobalStatBuffs {
    // type_key -> (stat_name -> procent acumulat)
    std::unordered_map<std::string,
                       std::unordered_map<std::string, float>> per_type_;

public:
    // Acumuleaza un buff (ex. add("antivirus", "damage_pct", 0.07)).
    void add(const std::string& type_key, const std::string& stat, float value) {
        per_type_[type_key][stat] += value;
    }

    // Procentul acumulat pentru (tip, stat); 0 daca nu s-a cumparat nimic.
    // Tower-ele multiplica baza din spec cu (1 + pct) cand calculeaza stats
    // efective (deci aditiv intre buff-uri, nu exponential).
    float pct(const std::string& type_key, const std::string& stat) const {
        auto t = per_type_.find(type_key);
        if (t == per_type_.end()) return 0.0f;
        auto s = t->second.find(stat);
        return (s == t->second.end()) ? 0.0f : s->second;
    }

    // Iterare pentru save/load.
    // cppcheck-suppress unusedFunction
    const std::unordered_map<std::string,
                             std::unordered_map<std::string, float>>&
    all() const { return per_type_; }
};
