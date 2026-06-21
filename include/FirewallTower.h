#pragma once
#include "Tower.h"
#include <memory>
#include <vector>

class FirewallTower : public Tower {
    // per-instance state (HP-ul curent scade pe parcursul jocului)
    float currentHP;

    // ability flags
    bool reflectiveShield;
    bool armored;

    // REFLECTIVE_SHIELD: cooldown intre apeluri (5s default).
    float shield_cooldown_ = 0.0f;

    // Mythic ShieldedRunner: la contact cu scutul PLIN, firewall-ul sarjeaza
    // pe drum spre spawn, lovind tot ce traverseaza (% din HP curent + stun).
    // Anti-cheese: scutul se goleste instant la activare, deci urmatoarea
    // sarja vine abia dupa ce regen-ul il umple la loc.
    bool  shielded_runner_ = false;
    bool  charging_        = false;
    float charge_idx_      = 0.0f;   // index float in charge_cells_, scade spre 0
    std::vector<std::pair<int, int>> charge_cells_;   // path expandat la activare
    std::vector<int> charge_hit_ids_;                  // fiecare inamic lovit o data

    void blockEnemy(Enemy& enemy);
    void startCharge(const std::vector<std::pair<int, int>>& path);
    void tickCharge(std::vector<Enemy>& enemies, float deltaTime);

public:
    FirewallTower(const TowerSpec& spec, int col, int row);

    void update(std::vector<Enemy>& enemies, float deltaTime,
                const GlobalStatBuffs& buffs,
                const std::vector<std::pair<int, int>>& path,
                std::vector<Shot>& out_shots) override;
    char getDisplayChar() const override;
    std::unique_ptr<Tower> clone() const override;

    void applyAbility(AbilityType a) override;
    void applyMythic(MythicType m) override;
    const char* mythicBadge() const override;
    std::vector<AbilityType> getAppliedAbilities() const override;

    // In timpul sarjei, turnul se deseneaza la pozitia curenta de pe drum.
    std::pair<float, float> visualPosition() const override;

protected:
    void displayDetails(std::ostream& os) const override;
};
