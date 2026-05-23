#include "AbilityEvolution.h"
#include "Tower.h"
#include "AntivirusTower.h"
#include "AdblockerTower.h"
#include "GameException.h"
#include <algorithm>   // std::minmax
#include <utility>

AbilityEvolution::AbilityEvolution(std::string name, int cost, Rarity rarity, AbilityType ability)
    : Evolution(std::move(name), cost, rarity), ability(ability) {}


// T3 refactor: majoritatea ability-urilor merg virtual  (Tower::applyAbility override pe derivate).
//dynamic_cast ca sa apelam setter-ul concret pe Antivirus/Adblocker (singurele care suporta knockback).

void AbilityEvolution::apply(const EvolutionContext& ctx) {
    if (!ctx.target_tower) {
        throw IncompatibleEvolutionException(
            "AbilityEvolution.apply: invalid target");
    }

    if (ability == AbilityType::KNOCKBACK_EVERY_3) {
        if (auto* anti = dynamic_cast<AntivirusTower*>(ctx.target_tower)) {
            anti->setKnockbackInterval(3);
            return;
        }
        if (auto* adb = dynamic_cast<AdblockerTower*>(ctx.target_tower)) {
            adb->setKnockbackInterval(3);
            return;
        }
        throw IncompatibleEvolutionException(
            "Knockback: incompatible evolution for this tower");
    }

    // Restul virtual dispatch
    ctx.target_tower->applyAbility(ability);
}

std::unique_ptr<Evolution> AbilityEvolution::clone() const {
    return std::make_unique<AbilityEvolution>(*this);
}

AbilityEvolution::AbilityType AbilityEvolution::getAbility() const { return ability; }

// 3 combinatii hardcodate. Restul perechilor returneaza false.
// std::minmax normalizeaza ordinea ca sa nu mai scriem (a,b) si (b,a) separat
// cppcheck-suppress unusedFunction // T3
bool AbilityEvolution::canCombine(AbilityType a, AbilityType b) {
    if (a == b) return false;
    auto p = std::minmax(a, b);
    auto x = p.first;
    auto y = p.second;

    // PHOENIX_BARRAGE = DOUBLE_SHOT + FIRE_TRAIL
    if (x == AbilityType::DOUBLE_SHOT && y == AbilityType::FIRE_TRAIL) return true;
    // ROVING_BRUISER = KNOCKBACK_EVERY_3 + MOVABLE
    if (x == AbilityType::KNOCKBACK_EVERY_3 && y == AbilityType::MOVABLE) return true;
    // SHIELDED_RUNNER = REFLECTIVE_SHIELD + MOVABLE
    if (x == AbilityType::REFLECTIVE_SHIELD && y == AbilityType::MOVABLE) return true;

    return false;
}

static const char* abilityToStr(AbilityEvolution::AbilityType a) {
    using A = AbilityEvolution::AbilityType;
    switch (a) {
        case A::MULTI_TARGET:      return "MultiTarget";
        case A::BIGGER_AURA:       return "BiggerAura";
        case A::ARMORED:           return "Armored";
        case A::DOUBLE_SHOT:       return "DoubleShot";
        case A::FIRE_TRAIL:        return "FireTrail";
        case A::KNOCKBACK_EVERY_3: return "Knockback/3";
        case A::REFLECTIVE_SHIELD: return "ReflectShield";
        case A::MOVABLE:           return "Movable";
    }
    return "?";
}

void AbilityEvolution::displayDetails(std::ostream& os) const {
    os << " ability:" << abilityToStr(ability);
}
