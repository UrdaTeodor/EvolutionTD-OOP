#include "FirewallTower.h"
#include <algorithm>
#include <cmath>
#include <iostream>

FirewallTower::FirewallTower(int col, int row)
    : Tower("Firewall", 60, col, row, 0.0f),
      currentHP(80.0f), maxHP(80.0f), regenRate(5.0f),
      reflectiveShield(false), armored(false) {}

std::unique_ptr<Tower> makeFirewall(int col, int row) {
    return std::make_unique<FirewallTower>(col, row);
}

bool FirewallTower::requiresPath() const { return true; }

void FirewallTower::blockEnemy(Enemy& enemy) {
    if (enemy.getCurrentHealth() <= currentHP) {
        currentHP -= enemy.getCurrentHealth();
        enemy.takeDamage(enemy.getCurrentHealth());
        std::cout << "  " << getName() << " absorbed " << enemy.getName()
                  << "! (Firewall HP: " << static_cast<int>(currentHP)
                  << "/" << static_cast<int>(maxHP) << ")\n";
    }
}

void FirewallTower::update(std::vector<Enemy>& enemies, float deltaTime) {
    currentHP = std::min(currentHP + regenRate * deltaTime, maxHP);

    for (auto& enemy : enemies) {
        if (!enemy.isAlive()) continue;
        float dx = enemy.getX() - static_cast<float>(getX());
        float dy = enemy.getY() - static_cast<float>(getY());
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist < 0.6f) {
            blockEnemy(enemy);
        }
    }
}

char FirewallTower::getDisplayChar() const { return 'F'; }

std::unique_ptr<Tower> FirewallTower::clone() const {
    return std::make_unique<FirewallTower>(*this);
}

// stat buffs: buff-ul de maxHP creste si currentHP proportional
void FirewallTower::buffMaxHP(float pct) {
    float ratio = currentHP / maxHP;
    maxHP    *= (1.0f + pct);
    currentHP = maxHP * ratio;
}
void FirewallTower::buffRegenRate(float pct) { regenRate *= (1.0f + pct); }

void FirewallTower::enableReflectiveShield() { reflectiveShield = true; }
void FirewallTower::enableArmored()          { armored = true; }

void FirewallTower::displayDetails(std::ostream& os) const {
    os << " HP:" << static_cast<int>(currentHP) << "/" << static_cast<int>(maxHP);
    if (reflectiveShield) os << " [ReflectShield]";
    if (armored)          os << " [Armored]";
}
