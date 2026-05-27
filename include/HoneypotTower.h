#pragma once
#include "Tower.h"
#include <memory>
#include <vector>

class HoneypotTower : public Tower {
    int biggerAuraStacks = 0;

public:
    HoneypotTower(const TowerSpec& spec, int col, int row);

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

std::unique_ptr<Tower> makeHoneypot(const TowerSpec& spec, int col, int row);
