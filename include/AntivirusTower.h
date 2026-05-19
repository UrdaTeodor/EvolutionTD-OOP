#pragma once
#include "Tower.h"
#include <memory>
#include <utility>

class AntivirusTower : public Tower {
    float damage;
    float attackSpeed;
    float projectileSpeed;
    float attackCooldown;

    // evo flags
    bool doubleShot;
    bool fireTrail;
    bool multiTarget;
    int knockbackInterval;    // 0 = off, N = la fiecare al N-lea shot
    int shotCounter;          // contor pentru knockback periodic

    bool isInRange(const Enemy& enemy) const;
    void attackEnemy(Enemy& enemy);
    std::pair<float, float> calculateInterceptPoint(const Enemy& enemy) const;

public:
    AntivirusTower(int col, int row);

    void update(std::vector<Enemy>& enemies, float deltaTime) override;
    char getDisplayChar() const override;
    std::unique_ptr<Tower> clone() const override;

    // evo buffs cu dynamic cast)
    void buffDamage(float pct);
    void buffAttackSpeed(float pct);

    // evo buffs
    void enableDoubleShot();
    void enableFireTrail();
    void enableMultiTarget();
    void enableKnockback(int interval);

protected:
    void displayDetails(std::ostream& os) const override;
};

std::unique_ptr<Tower> makeAntivirus(int col, int row);
