#pragma once
#include "Tower.h"
#include <memory>

// FirewallTower : public Tower => "este un fel de Tower".
// Singurul turn care sta PE DRUM (suprascrie requiresPath() -> true).
class FirewallTower : public Tower {
    float currentHP;
    float maxHP;
    float regenRate;

    // ability flags
    bool reflectiveShield;
    bool armored;

    void blockEnemy(Enemy& enemy);

public:
    FirewallTower(int col, int row);

    void update(std::vector<Enemy>& enemies, float deltaTime) override;
    char getDisplayChar() const override;
    std::unique_ptr<Tower> clone() const override;
    bool requiresPath() const override;

    // stat buffs (apelate din StatEvolution::apply prin dynamic_cast)
    void buffMaxHP(float pct);
    void buffRegenRate(float pct);

    // ability enablers (apelate din AbilityEvolution::apply)
    void enableReflectiveShield();
    void enableArmored();

protected:
    void displayDetails(std::ostream& os) const override;
};

std::unique_ptr<Tower> makeFirewall(int col, int row);
