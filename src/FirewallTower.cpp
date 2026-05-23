#include "FirewallTower.h"
#include "GlobalStatBuffs.h"
#include "GameException.h"
#include <algorithm>
#include <cmath>
#include <iostream>

FirewallTower::FirewallTower(const TowerSpec& spec, int col, int row)
    : Tower(spec, "firewall", col, row),
      currentHP(spec.max_hp),
      reflectiveShield(false), armored(false) {}

std::unique_ptr<Tower> makeFirewall(const TowerSpec& spec, int col, int row) {
    return std::make_unique<FirewallTower>(spec, col, row);
}

void FirewallTower::blockEnemy(Enemy& enemy) {
    if (enemy.getCurrentHealth() <= currentHP) {
        currentHP -= enemy.getCurrentHealth();
        enemy.takeDamage(enemy.getCurrentHealth());
        std::cout << "  " << getName() << " absorbed " << enemy.getName()
                  << "! (Firewall HP: " << static_cast<int>(currentHP)
                  << "/" << static_cast<int>(spec().max_hp) << ")\n";
    }
}

void FirewallTower::update(std::vector<Enemy>& enemies, float deltaTime,
                           const GlobalStatBuffs& buffs) {
    // Stats efective cu buff aplicat
    float max_hp_effective = spec().max_hp * (1.0f + buffs.for_type(getTypeKey()).max_hp_pct);
    float regen_effective  = spec().regen_rate * (1.0f + buffs.for_type(getTypeKey()).regen_pct);

    currentHP = std::min(currentHP + regen_effective * deltaTime, max_hp_effective);

    float block_r = spec().block_radius;
    for (auto& enemy : enemies) {
        if (!enemy.isAlive()) continue;
        float dx = enemy.getX() - static_cast<float>(getX());
        float dy = enemy.getY() - static_cast<float>(getY());
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist < block_r) {
            blockEnemy(enemy);
        }
    }
}

char FirewallTower::getDisplayChar() const { return 'F'; }

std::unique_ptr<Tower> FirewallTower::clone() const {
    return std::make_unique<FirewallTower>(*this);
}

void FirewallTower::applyAbility(AbilityType a) {
    switch (a) {
        case AbilityType::ARMORED:           armored          = true; break;
        case AbilityType::REFLECTIVE_SHIELD: reflectiveShield = true; break;
        case AbilityType::MOVABLE:           enableMovable();         break;
        default:
            Tower::applyAbility(a);
    }
}

void FirewallTower::displayDetails(std::ostream& os) const {
    os << " HP:" << static_cast<int>(currentHP) << "/" << static_cast<int>(spec().max_hp);
    if (reflectiveShield) os << " [ReflectShield]";
    if (armored)          os << " [Armored]";
}
