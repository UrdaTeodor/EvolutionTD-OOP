#pragma once
#include "Tower.h"
#include <memory>

class HoneypotTower : public Tower {
    bool biggerAura;

public:
    HoneypotTower(const TowerSpec& spec, int col, int row);

    void update(std::vector<Enemy>& enemies, float deltaTime,
                const GlobalStatBuffs& buffs) override;
    char getDisplayChar() const override;
    std::unique_ptr<Tower> clone() const override;

    void applyAbility(AbilityType a) override;

protected:
    void displayDetails(std::ostream& os) const override;
};

std::unique_ptr<Tower> makeHoneypot(const TowerSpec& spec, int col, int row);
