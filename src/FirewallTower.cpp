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

void FirewallTower::blockEnemy(Enemy& enemy) {
    // Standard block: absoarbe enemy.HP din proprul HP. ARMORED reduce costul la 50%.
    if (enemy.getCurrentHealth() <= currentHP) {
        float cost = armored ? (enemy.getCurrentHealth() * 0.5f) : enemy.getCurrentHealth();
        currentHP -= cost;
        enemy.takeDamage(enemy.getCurrentHealth());   // enemy ucis
    }
}

void FirewallTower::update(std::vector<Enemy>& enemies, float deltaTime,
                           const GlobalStatBuffs& buffs,
                           const std::vector<std::pair<int, int>>& path) {
    // Stats efective cu buff aplicat
    float max_hp_effective = spec().max_hp    * (1.0f + buffs.for_type(getTypeKey()).max_hp_pct);
    float regen_effective  = spec().regen_rate * (1.0f + buffs.for_type(getTypeKey()).regen_pct);

    currentHP = std::min(currentHP + regen_effective * deltaTime, max_hp_effective);
    shield_cooldown_ = std::max(0.0f, shield_cooldown_ - deltaTime);

    constexpr float SHIELD_COOLDOWN_SEC = 5.0f;
    constexpr int   SHIELD_PUSH_CELLS   = 3;

    float block_r = spec().block_radius;
    for (auto& enemy : enemies) {
        if (!enemy.isAlive()) continue;
        float dx = enemy.getX() - static_cast<float>(getX());
        float dy = enemy.getY() - static_cast<float>(getY());
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist >= block_r) continue;

        // REFLECTIVE_SHIELD: daca cooldown ready, push enemy in loc sa absorbim damage.
        if (reflectiveShield && shield_cooldown_ <= 0.0f) {
            enemy.pushBack(SHIELD_PUSH_CELLS, path);
            shield_cooldown_ = SHIELD_COOLDOWN_SEC;
        } else {
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

std::vector<AbilityType> FirewallTower::getAppliedAbilities() const {
    auto result = Tower::getAppliedAbilities();
    if (armored)          result.push_back(AbilityType::ARMORED);
    if (reflectiveShield) result.push_back(AbilityType::REFLECTIVE_SHIELD);
    return result;
}
