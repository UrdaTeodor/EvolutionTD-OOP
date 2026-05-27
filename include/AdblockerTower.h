#pragma once
#include "Tower.h"
#include <memory>
#include <utility>
#include <vector>

class AdblockerTower : public Tower {
    int doubleShotStacks  = 0;
    int fireTrailStacks   = 0;
    int multiTargetStacks = 0;
    int knockbackInterval = 0;
    int shotCounter       = 0;
    float attackCooldown  = 0.0f;

    bool isInRange(const Enemy& enemy, float effectiveRange) const;
    void attackEnemy(Enemy& enemy);
    std::pair<float, float> calculateInterceptPoint(const Enemy& enemy) const;

public:
    AdblockerTower(const TowerSpec& spec, int col, int row);

    void update(std::vector<Enemy>& enemies, float deltaTime,
                 const GlobalStatBuffs& buffs,
                 const std::vector<std::pair<int, int>>& path) override;
    char getDisplayChar() const override;
    std::unique_ptr<Tower> clone() const override;

    void applyAbility(AbilityType a) override;
    std::vector<AbilityType> getAppliedAbilities() const override;

    // Apelat din AbilityEvolution::apply cu dynamic_cast
    void setKnockbackInterval(int N);

protected:
    void displayDetails(std::ostream& os) const override;
};

std::unique_ptr<Tower> makeAdblocker(const TowerSpec& spec, int col, int row);
