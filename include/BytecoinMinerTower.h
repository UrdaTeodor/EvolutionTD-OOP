#pragma once
#include "Tower.h"
#include <memory>

//doar genereaza venit pasiv.
class BytecoinMinerTower : public Tower {
public:
    BytecoinMinerTower(const TowerSpec& spec, int col, int row);

    void update(std::vector<Enemy>& enemies, float deltaTime,
                const GlobalStatBuffs& buffs,
                const std::vector<std::pair<int, int>>& path) override;
    char getDisplayChar() const override;
    std::unique_ptr<Tower> clone() const override;
    int collectIncome(const GlobalStatBuffs& buffs) const override;

    void applyAbility(AbilityType a) override;

protected:
    void displayDetails(std::ostream& os) const override;
};

std::unique_ptr<Tower> makeBytecoinMiner(const TowerSpec& spec, int col, int row);
