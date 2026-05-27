#include "AdblockerTower.h"
#include "GlobalStatBuffs.h"
#include "GameException.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

AdblockerTower::AdblockerTower(const TowerSpec& spec, int col, int row)
    : Tower(spec, "adblocker", col, row) {}

bool AdblockerTower::isInRange(const Enemy& enemy, float effectiveRange) const {
    float dx = enemy.getX() - static_cast<float>(getX());
    float dy = enemy.getY() - static_cast<float>(getY());
    return std::sqrt(dx * dx + dy * dy) <= effectiveRange;
}

void AdblockerTower::update(std::vector<Enemy>& enemies, float deltaTime,
                            const GlobalStatBuffs& buffs,
                            const std::vector<std::pair<int, int>>& path) {
    attackCooldown -= deltaTime;
    if (attackCooldown > 0.0f) return;

    const auto& tb = buffs.for_type(getTypeKey());
    float damage_effective       = spec().damage       * (1.0f + tb.damage_pct);
    float attack_speed_effective = spec().attack_speed * (1.0f + tb.attack_speed_pct);
    float range                  = effectiveRange(buffs);

    int max_targets = 1 + multiTargetStacks;
    std::vector<Enemy*> targets;
    {
        std::vector<std::pair<float, Enemy*>> in_range;
        for (auto& enemy : enemies) {
            if (!enemy.isAlive() || !isInRange(enemy, range)) continue;
            float dx = enemy.getX() - static_cast<float>(getX());
            float dy = enemy.getY() - static_cast<float>(getY());
            in_range.push_back({ std::sqrt(dx * dx + dy * dy), &enemy });
        }
        std::sort(in_range.begin(), in_range.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });
        for (int i = 0; i < max_targets && i < static_cast<int>(in_range.size()); ++i) {
            targets.push_back(in_range[i].second);
        }
    }
    if (targets.empty()) return;

    int   attacks_per_target = 1 + doubleShotStacks;
    float damage_per_attack  = damage_effective;
    bool  has_double_shot    = doubleShotStacks > 0;

    for (Enemy* target : targets) {
        Enemy* pierce_target = nullptr;
        if (has_double_shot) {
            float bestDist = 1e9f;
            for (auto& e : enemies) {
                if (!e.isAlive() || !isInRange(e, range)) continue;
                bool in_primary = false;
                for (const Enemy* t : targets) if (&e == t) { in_primary = true; break; }
                if (in_primary) continue;
                float dx = e.getX() - static_cast<float>(getX());
                float dy = e.getY() - static_cast<float>(getY());
                float d  = std::sqrt(dx*dx + dy*dy);
                if (d < bestDist) { bestDist = d; pierce_target = &e; }
            }
        }

        for (int a = 0; a < attacks_per_target; ++a) {
            target->takeDamage(damage_per_attack);
            ++shotCounter;
            if (knockbackInterval > 0 && shotCounter % knockbackInterval == 0) {
                target->pushBack(1, path);
            }
            if (pierce_target && pierce_target->isAlive()) {
                pierce_target->takeDamage(damage_per_attack);
            }
        }
        if (fireTrailStacks > 0) {
            float dps = damage_effective * 0.4f * fireTrailStacks;
            target->applyFireTrail(dps, 4.0f);
            if (pierce_target && pierce_target->isAlive()) {
                pierce_target->applyFireTrail(dps, 4.0f);
            }
        }
    }

    attackCooldown = 1.0f / attack_speed_effective;
}

char AdblockerTower::getDisplayChar() const { return 'D'; }

std::unique_ptr<Tower> AdblockerTower::clone() const {
    return std::make_unique<AdblockerTower>(*this);
}

void AdblockerTower::applyAbility(AbilityType a) {
    switch (a) {
        case AbilityType::DOUBLE_SHOT:        ++doubleShotStacks;  break;
        case AbilityType::FIRE_TRAIL:         ++fireTrailStacks;   break;
        case AbilityType::MULTI_TARGET:       ++multiTargetStacks; break;
        case AbilityType::MOVABLE:            enableMovable();     break;

        default:
            Tower::applyAbility(a);
    }
}

void AdblockerTower::setKnockbackInterval(int N) {
    knockbackInterval = N;
}

std::vector<AbilityType> AdblockerTower::getAppliedAbilities() const {
    auto result = Tower::getAppliedAbilities();
    for (int i = 0; i < multiTargetStacks; ++i) result.push_back(AbilityType::MULTI_TARGET);
    for (int i = 0; i < doubleShotStacks; ++i)  result.push_back(AbilityType::DOUBLE_SHOT);
    for (int i = 0; i < fireTrailStacks; ++i)   result.push_back(AbilityType::FIRE_TRAIL);
    if (knockbackInterval > 0) result.push_back(AbilityType::KNOCKBACK_EVERY_3);
    return result;
}

void AdblockerTower::displayDetails(std::ostream& os) const {
    os << " dmg:" << spec().damage << " aspd:" << spec().attack_speed;
    if (doubleShotStacks  > 0) os << " [DoubleShot x"  << doubleShotStacks  << "]";
    if (fireTrailStacks   > 0) os << " [FireTrail x"   << fireTrailStacks   << "]";
    if (multiTargetStacks > 0) os << " [MultiTarget x" << multiTargetStacks << "]";
    if (knockbackInterval > 0) os << " [Knockback/" << knockbackInterval << "]";
}
