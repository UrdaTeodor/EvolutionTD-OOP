#pragma once
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "AbilityType.h"
#include "Evolution.h"

// POD-uri citite din data/evolutions.json. Folosite de EvolutionFactory
// si de ShopPanel pentru a afisa cardurile pe care le poate cumpara playerul.

// MiniStatSpec
// Buff procentual aplicat pe TowerType (global) imediat la cumparare.
// stat_field e direct cheia de stat din GlobalStatBuffs (GlobalStatBuffs.h):
//   "damage_pct", "range_pct", "attack_speed_pct", "max_hp_pct", "regen_pct",
//   "slow_pct", "income_pct" — fara mapare la campuri fixe, e cheia in harta.
struct MiniStatSpec {
    std::string name;
    int cost = 0;
    std::string stat_field;
    float stat_value = 0.0f;
    // Populat la sample (ShopPanel::refresh): random tower-type din cei afectati
    // de stat_field (ex. damage_pct -> "antivirus" sau "adblocker"). 
    std::string target_type;
};

inline void from_json(const nlohmann::json& j, MiniStatSpec& s) {
    j.at("name").get_to(s.name);
    j.at("cost").get_to(s.cost);
    j.at("stat_field").get_to(s.stat_field);
    j.at("stat_value").get_to(s.stat_value);
}

// MajorEvolutionSpec
// Doua tipuri:
//   stat -> aplicat pe GlobalStatBuffs (Rare)
//   ability -> genereaza EvolutionToken in inventar (Epic / Legendary)
struct MajorEvolutionSpec {
    enum class Kind { STAT, ABILITY };

    std::string name;
    int cost = 0;
    Evolution::Rarity rarity = Evolution::Rarity::RARE;
    Kind kind = Kind::STAT;

    std::string stat_field;
    float stat_value = 0.0f;

    AbilityType ability = AbilityType::MULTI_TARGET;
    std::string target_type;
};

inline Evolution::Rarity rarityFromString(const std::string& s) {
    // Case-insensitive: writer-ul (rarityToString) scrie Title Case ("Legendary")
    // iar JSON-ul de date scrie UPPER ("LEGENDARY"). Normalizam la UPPER ca
    // ambele surse, inclusiv save-urile vechi deja scrise pe disc, sa se
    // incarce, nu sa pice la "Continue".
    std::string u = s;
    std::transform(u.begin(), u.end(), u.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    if (u == "MINI")      return Evolution::Rarity::MINI;
    if (u == "RARE")      return Evolution::Rarity::RARE;
    if (u == "EPIC")      return Evolution::Rarity::EPIC;
    if (u == "LEGENDARY") return Evolution::Rarity::LEGENDARY;
    if (u == "MYTHIC")    return Evolution::Rarity::MYTHIC;
    throw std::invalid_argument("Rarity necunoscuta: " + s);
}

// cppcheck-suppress unusedFunction
inline const char* rarityToString(Evolution::Rarity r) {
    switch (r) {
        case Evolution::Rarity::MINI:      return "Mini";
        case Evolution::Rarity::RARE:      return "Rare";
        case Evolution::Rarity::EPIC:      return "Epic";
        case Evolution::Rarity::LEGENDARY: return "Legendary";
        case Evolution::Rarity::MYTHIC:    return "Mythic";
    }
    return "uhh";
}

inline void from_json(const nlohmann::json& j, MajorEvolutionSpec& s) {
    j.at("name").get_to(s.name);
    j.at("cost").get_to(s.cost);

    std::string rar_str;
    j.at("rarity").get_to(rar_str);
    s.rarity = rarityFromString(rar_str);

    std::string kind_str;
    j.at("kind").get_to(kind_str);
    if (kind_str == "STAT") {
        s.kind = MajorEvolutionSpec::Kind::STAT;
        j.at("stat_field").get_to(s.stat_field);
        j.at("stat_value").get_to(s.stat_value);
    } 
    else if (kind_str == "ABILITY") {
        s.kind = MajorEvolutionSpec::Kind::ABILITY;
        std::string ab_str;
        j.at("ability").get_to(ab_str);
        s.ability = stringToAbility(ab_str);
    } 
    else {
        throw std::invalid_argument(
            "MajorEvolutionSpec.kind invalid (asteapta STAT sau ABILITY): " + kind_str);
    }
}

// MythicRecipeSpec
// ability A + ability B 
// Mythic nu apare ca pick aleator in shop; player il crafteaza cand are token de evolutie mythica
// Legendary care match-uiesc o reteta.
struct MythicRecipeSpec {
    AbilityType ingredient_a;
    AbilityType ingredient_b;
    std::string result_name;
};

// cppcheck-suppress unusedFunction
inline void from_json(const nlohmann::json& j, MythicRecipeSpec& s) {
    std::string a_str, b_str;
    j.at("ingredient_a").get_to(a_str);
    j.at("ingredient_b").get_to(b_str);
    s.ingredient_a = stringToAbility(a_str);
    s.ingredient_b = stringToAbility(b_str);
    j.at("result_name").get_to(s.result_name);
}
