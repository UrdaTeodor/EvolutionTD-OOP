#pragma once
#include "Tower.h"
#include <memory>
#include <utility>

class AdblockerTower : public Tower {
    bool doubleShot;
    bool fireTrail;
    bool multiTarget;
    int  knockbackInterval;
    int  shotCounter;
    float attackCooldown;

    bool isInRange(const Enemy& enemy, float effectiveRange) const;
    void attackEnemy(Enemy& enemy);
    std::pair<float, float> calculateInterceptPoint(const Enemy& enemy) const;

public:
    AdblockerTower(const TowerSpec& spec, int col, int row);

    void update(std::vector<Enemy>& enemies, float deltaTime,
                const GlobalStatBuffs& buffs) override;
    char getDisplayChar() const override;
    std::unique_ptr<Tower> clone() const override;

    void applyAbility(AbilityType a) override;

    // Apelat din AbilityEvolution::apply cu dynamic_cast (T2).
    void setKnockbackInterval(int N);

protected:
    void displayDetails(std::ostream& os) const override;
};

std::unique_ptr<Tower> makeAdblocker(const TowerSpec& spec, int col, int row);
