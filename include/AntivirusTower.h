#pragma once
#include "Tower.h"
#include <memory>
#include <utility>

class AntivirusTower : public Tower {

    bool doubleShot;
    bool fireTrail;
    bool multiTarget;
    int  knockbackInterval;   // 0 = off, N = la fiecare al N-lea shot
    int  shotCounter;
    float attackCooldown;     // timer interior

    bool isInRange(const Enemy& enemy, float effectiveRange) const;
    void attackEnemy(Enemy& enemy);
    std::pair<float, float> calculateInterceptPoint(const Enemy& enemy) const;

public:
    AntivirusTower(const TowerSpec& spec, int col, int row);

    void update(std::vector<Enemy>& enemies, float deltaTime,
                const GlobalStatBuffs& buffs) override;
    char getDisplayChar() const override;
    std::unique_ptr<Tower> clone() const override;

    // T3 refactor: dynamic_cast in AbilityEvolution dispare pentru majoritatea evo-urilor;
    // Tower decide singur ce suporta(exceptie KNOCKBACK_EVERY_3)
    void applyAbility(AbilityType a) override;

    void setKnockbackInterval(int N);

protected:
    void displayDetails(std::ostream& os) const override;
};

std::unique_ptr<Tower> makeAntivirus(const TowerSpec& spec, int col, int row);
