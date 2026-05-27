#include "HoneypotTower.h"
#include "GlobalStatBuffs.h"
#include "GameException.h"
#include <cmath>

HoneypotTower::HoneypotTower(const TowerSpec& spec, int col, int row)
    : Tower(spec, "honeypot", col, row) {}

void HoneypotTower::update(std::vector<Enemy>& enemies, float /*deltaTime*/,
                           const GlobalStatBuffs& buffs,
                           const std::vector<std::pair<int, int>>& /*path*/) {
    float range = effectiveRange(buffs);
    if (biggerAuraStacks > 0) range *= (1.0f + biggerAuraStacks);

    float slow = spec().slow_factor;
    if (biggerAuraStacks > 0) {
        slow = 1.0f - (1.0f - slow) * (1.0f + 0.5f * biggerAuraStacks);
    }

    float slow_pct = buffs.for_type(getTypeKey()).slow_pct;
    slow = slow * (1.0f - slow_pct);
    if (slow < 0.0f) slow = 0.0f;

    for (auto& enemy : enemies) {
        if (!enemy.isAlive()) continue;
        float dx = enemy.getX() - static_cast<float>(getX());
        float dy = enemy.getY() - static_cast<float>(getY());
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist <= range) {
            enemy.applySlow(slow);
        }
    }
}

char HoneypotTower::getDisplayChar() const { return 'H'; }

std::unique_ptr<Tower> HoneypotTower::clone() const {
    return std::make_unique<HoneypotTower>(*this);
}

void HoneypotTower::applyAbility(AbilityType a) {
    switch (a) {
        case AbilityType::BIGGER_AURA: ++biggerAuraStacks;  break;
        case AbilityType::MOVABLE:     enableMovable();     break;
        default:
            Tower::applyAbility(a);
    }
}

void HoneypotTower::displayDetails(std::ostream& os) const {
    os << " slow:" << static_cast<int>((1.0f - spec().slow_factor) * 100.0f) << "%"
       << " range:" << spec().range;
    if (biggerAuraStacks > 0) os << " [BiggerAura x" << biggerAuraStacks << "]";
}

std::vector<AbilityType> HoneypotTower::getAppliedAbilities() const {
    auto result = Tower::getAppliedAbilities();
    for (int i = 0; i < biggerAuraStacks; ++i) result.push_back(AbilityType::BIGGER_AURA);
    return result;
}
