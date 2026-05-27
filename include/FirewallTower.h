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

    void blockEnemy(Enemy& enemy);

public:
    FirewallTower(const TowerSpec& spec, int col, int row);

    void update(std::vector<Enemy>& enemies, float deltaTime,
                const GlobalStatBuffs& buffs,
                const std::vector<std::pair<int, int>>& path) override;
    char getDisplayChar() const override;
    std::unique_ptr<Tower> clone() const override;

    void applyAbility(AbilityType a) override;
    std::vector<AbilityType> getAppliedAbilities() const override;

protected:
    void displayDetails(std::ostream& os) const override;
};
