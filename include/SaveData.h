#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct SaveData {
    //  Run state
    std::string map_id;
    int wave_number        = 1;
    int player_hp          = 0;
    int money              = 0;
    int total_kills        = 0;
    int total_money_earned = 0;
    int player_weight      = 0;
    bool endless_active    = false;
    int minis_bought       = 0;   // escaladare pret Mini (v0.4)

    //  Towers
    struct TowerEntry {
        std::string type_key;
        int col = 0;
        int row = 0;
        std::vector<std::string> applied_abilities;
        int token_investment = 0;
        std::string targeting = "FIRST";   // TargetingMode (v0.4)
        std::string mythic;                // numele mythic-ului aplicat, gol = niciunul (v0.4)
    };
    std::vector<TowerEntry> towers;

    //  GlobalStatBuffs per tower type
    struct BuffEntry {
        std::string type_key;
        float damage_pct        = 0.0f;
        float range_pct         = 0.0f;
        float attack_speed_pct  = 0.0f;
        float max_hp_pct        = 0.0f;
        float regen_pct         = 0.0f;
        float slow_pct          = 0.0f;
        float income_pct        = 0.0f;
    };
    std::vector<BuffEntry> buffs;

    //  ShopPanel offer state (cu bought flag, anti-RNG-manipulation) ----
    struct MiniSlot {
        std::string name;
        int cost = 0;
        std::string stat_field;
        float stat_value = 0.0f;
        std::string target_type;
        bool bought = false;
    };
    std::vector<MiniSlot> mini_offer;

    struct MajorSlot {
        std::string name;
        int cost = 0;
        std::string rarity;
        std::string kind;
        std::string stat_field;
        float stat_value = 0.0f;
        std::string ability;
        std::string target_type;
        bool bought = false;
    };
    std::vector<MajorSlot> major_offer;

    //  Tokens in inventar
    struct TokenEntry {
        std::string name;
        std::string ability;
        std::string rarity;
        int cost = 0;
        bool is_wildcard = false;      // v0.4.1: Mythic Token universal (loterie)
    };
    std::vector<TokenEntry> tokens;

    //  Loterie / gauntlet (v0.4.1)
    int  lottery_offers_made = 0;
    bool lottery_accepted    = false;
    int  gauntlet_waves_left = 0;

    // RNG state 
    // mt19937 serializat prin operator<< 
    // La load: std::istringstream(rng_state) >> rng;
    std::string rng_state;
};

// nlohmann macros genereaza automat to_json / from_json (folosesc ADL).
// Varianta WITH_DEFAULT ca save-urile vechi (fara campul "targeting") sa se
// incarce in continuare, cu default-ul din struct.
// cppcheck-suppress [unknownMacro, unreadVariable]
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SaveData::TowerEntry,
    type_key, col, row, applied_abilities, token_investment, targeting, mythic)

// cppcheck-suppress unknownMacro
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SaveData::BuffEntry,
    type_key, damage_pct, range_pct, attack_speed_pct,
    max_hp_pct, regen_pct, slow_pct, income_pct)

// cppcheck-suppress unknownMacro
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SaveData::MiniSlot,
    name, cost, stat_field, stat_value, target_type, bought)

// cppcheck-suppress unknownMacro
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SaveData::MajorSlot,
    name, cost, rarity, kind, stat_field, stat_value, ability, target_type, bought)

// cppcheck-suppress [unknownMacro, unreadVariable]
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SaveData::TokenEntry,
    name, ability, rarity, cost, is_wildcard)

// cppcheck-suppress [unknownMacro, unreadVariable]
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SaveData,
    map_id, wave_number, player_hp, money,
    total_kills, total_money_earned, player_weight, endless_active, minis_bought,
    lottery_offers_made, lottery_accepted, gauntlet_waves_left,
    towers, buffs,
    mini_offer, major_offer, tokens,
    rng_state)
