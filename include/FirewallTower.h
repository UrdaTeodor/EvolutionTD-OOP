#pragma once
#include "Tower.h"
#include <memory>

// FirewallTower : public Tower
// Singurul turn cu HP per-instance (currentHP) si singurul care sta pe path
class FirewallTower : public Tower {
    // per-instance state (HP-ul curent scade pe parcursul jocului)
    float currentHP;

    // ability flags
    bool reflectiveShield;
    bool armored;

    void blockEnemy(Enemy& enemy);

public:
    FirewallTower(const TowerSpec& spec, int col, int row);

    void update(std::vector<Enemy>& enemies, float deltaTime,
                const GlobalStatBuffs& buffs) override;
    char getDisplayChar() const override;
    std::unique_ptr<Tower> clone() const override;

    void applyAbility(AbilityType a) override;

protected:
    void displayDetails(std::ostream& os) const override;
};

std::unique_ptr<Tower> makeFirewall(const TowerSpec& spec, int col, int row);
