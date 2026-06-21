#include "HoneypotTower.h"
#include "GlobalStatBuffs.h"
#include "GameException.h"
#include <cmath>

HoneypotTower::HoneypotTower(const TowerSpec& spec, int col, int row)
    : Tower(spec, "honeypot", col, row) {}

float HoneypotTower::auraRange(const GlobalStatBuffs& buffs) const {
    float range = effectiveRange(buffs);
    if (biggerAuraStacks > 0) range *= 1.5f;
    return range;
}

void HoneypotTower::update(std::vector<Enemy>& enemies, float /*deltaTime*/,
                           const GlobalStatBuffs& buffs,
                           const std::vector<std::pair<int, int>>& /*path*/,
                           std::vector<Shot>& /*out_shots*/) {
    float range = auraRange(buffs);

    float slow = spec().slow_factor;
    if (biggerAuraStacks > 0) {
        slow = 1.0f - (1.0f - slow) * 1.5f;   // 20% -> 30% slow
    }

    float slow_pct = buffs.pct(getTypeKey(), "slow_pct");
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
        case AbilityType::BIGGER_AURA:
            // O aplicare degeaba e un token pierdut: respingem cu motiv.
            if (biggerAuraStacks > 0) {
                throw IncompatibleEvolutionException(
                    "BiggerAura nu se stackeaza (turnul o are deja).");
            }
            ++biggerAuraStacks;
            break;
        case AbilityType::AMPLIFY:     ++amplifyStacks_;     break;
        case AbilityType::OVERCLOCK:   ++overclockStacks_;   break;
        case AbilityType::MOVABLE:     enableMovable();      break;
        default:
            Tower::applyAbility(a);
    }
}

void HoneypotTower::displayDetails(std::ostream& os) const {
    os << " slow:" << static_cast<int>((1.0f - spec().slow_factor) * 100.0f) << "%"
       << " range:" << spec().range;
    if (biggerAuraStacks > 0) os << " [BiggerAura x" << biggerAuraStacks << "]";
    if (amplifyStacks_   > 0) os << " [Amplify x"    << amplifyStacks_   << "]";
    if (overclockStacks_ > 0) os << " [Overclock x"  << overclockStacks_ << "]";
}

std::vector<AbilityType> HoneypotTower::getAppliedAbilities() const {
    auto result = Tower::getAppliedAbilities();
    for (int i = 0; i < biggerAuraStacks; ++i) result.push_back(AbilityType::BIGGER_AURA);
    for (int i = 0; i < amplifyStacks_;   ++i) result.push_back(AbilityType::AMPLIFY);
    for (int i = 0; i < overclockStacks_; ++i) result.push_back(AbilityType::OVERCLOCK);
    return result;
}
