#include "FirewallTower.h"
#include "GlobalStatBuffs.h"
#include "GameException.h"
#include "PathUtils.h"
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

void FirewallTower::startCharge(const std::vector<std::pair<int, int>>& path) {
    charge_cells_ = expandPathCells(path);

    // Gaseste celula proprie pe drum (firewall-ul sta mereu PE drum).
    int own_idx = -1;
    for (int i = 0; i < static_cast<int>(charge_cells_.size()); ++i) {
        if (charge_cells_[i].second == getX() && charge_cells_[i].first == getY()) {
            own_idx = i;
            break;
        }
    }
    if (own_idx <= 0) return;   // la spawn sau drum necunoscut: nu are unde sarja

    charging_   = true;
    charge_idx_ = static_cast<float>(own_idx);
    charge_hit_ids_.clear();
    currentHP = 0.0f;   // scutul se goleste instant la activare (anti-cheese)
}

void FirewallTower::tickCharge(std::vector<Enemy>& enemies, float deltaTime) {
    constexpr float CHARGE_SPEED      = 12.0f;   // celule pe secunda
    constexpr float CHARGE_HIT_RADIUS = 0.9f;
    constexpr float CHARGE_DMG_PCT    = 0.20f;   // % din HP-ul CURENT (nu poate ucide)
    constexpr float CHARGE_STUN_SEC   = 1.5f;

    charge_idx_ -= CHARGE_SPEED * deltaTime;
    if (charge_idx_ <= 0.0f) {
        // A ajuns la spawn: sarja s-a terminat, turnul "revine" pe celula lui
        // (pozitia logica nu s-a schimbat niciodata, doar cea vizuala).
        charging_ = false;
        charge_hit_ids_.clear();
        return;
    }

    const auto& [row, col] = charge_cells_[static_cast<size_t>(charge_idx_)];
    for (auto& e : enemies) {
        if (!e.isAlive()) continue;
        if (std::find(charge_hit_ids_.begin(), charge_hit_ids_.end(), e.id())
                != charge_hit_ids_.end()) continue;
        float dx = e.getX() - static_cast<float>(col);
        float dy = e.getY() - static_cast<float>(row);
        if (std::sqrt(dx * dx + dy * dy) > CHARGE_HIT_RADIUS) continue;

        e.takeDamage(e.getCurrentHealth() * CHARGE_DMG_PCT);
        e.applyStun(CHARGE_STUN_SEC);
        charge_hit_ids_.push_back(e.id());
    }
}

void FirewallTower::update(std::vector<Enemy>& enemies, float deltaTime,
                           const GlobalStatBuffs& buffs,
                           const std::vector<std::pair<int, int>>& path,
                           std::vector<Shot>& /*out_shots*/) {
    // Stats efective cu buff aplicat
    float max_hp_effective = spec().max_hp    * (1.0f + buffs.pct(getTypeKey(), "max_hp_pct"));
    float regen_effective  = spec().regen_rate * (1.0f + buffs.pct(getTypeKey(), "regen_pct"));

    // In timpul sarjei, firewall-ul e "plecat": nu regenereaza si nu blocheaza.
    if (charging_) {
        tickCharge(enemies, deltaTime);
        return;
    }

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

        // ShieldedRunner: contact cu scutul PLIN declanseaza sarja.
        if (shielded_runner_ && currentHP >= max_hp_effective - 0.01f) {
            startCharge(path);
            if (charging_) return;
        }

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
        case AbilityType::ARMORED:
            if (armored) {
                throw IncompatibleEvolutionException(
                    "Armored nu se stackeaza (turnul il are deja).");
            }
            armored = true;
            break;
        case AbilityType::REFLECTIVE_SHIELD:
            if (reflectiveShield) {
                throw IncompatibleEvolutionException(
                    "ReflectShield nu se stackeaza (turnul il are deja).");
            }
            reflectiveShield = true;
            break;
        case AbilityType::MOVABLE:           enableMovable();         break;
        default:
            Tower::applyAbility(a);
    }
}

void FirewallTower::applyMythic(MythicType m) {
    if (mythicBadge() != nullptr) {
        throw IncompatibleEvolutionException(
            "Turnul are deja un Mythic (max 1 per turn).");
    }
    if (m == MythicType::SHIELDED_RUNNER) {
        shielded_runner_ = true;
    } else {
        Tower::applyMythic(m);   // throw incompatibil
    }
}

const char* FirewallTower::mythicBadge() const {
    return shielded_runner_ ? "ShieldedRunner" : nullptr;
}

std::pair<float, float> FirewallTower::visualPosition() const {
    if (charging_ && !charge_cells_.empty()) {
        size_t idx = static_cast<size_t>(std::max(0.0f, charge_idx_));
        if (idx < charge_cells_.size()) {
            return { static_cast<float>(charge_cells_[idx].second),
                     static_cast<float>(charge_cells_[idx].first) };
        }
    }
    return Tower::visualPosition();
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
