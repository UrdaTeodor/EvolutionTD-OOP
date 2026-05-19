#pragma once
#include "Tower.h"
#include <memory>
#include <utility>

class AdblockerTower : public Tower {
    float damage;
    float attackSpeed;
    float projectileSpeed;
    float attackCooldown;

  //evo flags
    bool doubleShot;
    bool fireTrail;
    bool multiTarget;
    int knockbackInterval;
    int shotCounter;

    bool isInRange(const Enemy& enemy) const;
    void attackEnemy(Enemy& enemy);
    std::pair<float, float> calculateInterceptPoint(const Enemy& enemy) const;

public:
    AdblockerTower(int col, int row);

    void update(std::vector<Enemy>& enemies, float deltaTime) override;
    char getDisplayChar() const override;
    std::unique_ptr<Tower> clone() const override;

    // evos
    void buffDamage(float pct);
    void buffAttackSpeed(float pct);

    // evos
    void enableDoubleShot();
    void enableFireTrail();
    void enableMultiTarget();
    void enableKnockback(int interval);

protected:
    void displayDetails(std::ostream& os) const override;
};

std::unique_ptr<Tower> makeAdblocker(int col, int row);
