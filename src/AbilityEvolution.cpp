#include "AbilityEvolution.h"
#include "Tower.h"
#include "ProjectileTower.h"
#include "GameException.h"
#include <utility>

AbilityEvolution::AbilityEvolution(std::string name, int cost, Rarity rarity, AbilityType ability)
    : Evolution(std::move(name), cost, rarity), ability(ability) {}


// T3 refactor majoritatea ability-urilor merg virtual  (Tower::applyAbility override pe derivate).
//dynamic_cast ca sa apelam setter-ul concret pe Antivirus/Adblocker (singurele care suporta knockback).

void AbilityEvolution::apply(const EvolutionContext& ctx) {
    if (!ctx.target_tower) {
        throw IncompatibleEvolutionException(
            "AbilityEvolution.apply: invalid target");
    }

    if (ability == AbilityType::KNOCKBACK_EVERY_3) {
        // Downcast cu sens: doar turnurile cu proiectile au knockback interval.
        if (auto* pt = dynamic_cast<ProjectileTower*>(ctx.target_tower)) {
            pt->setKnockbackInterval(3);
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

void AbilityEvolution::displayDetails(std::ostream& os) const {
    os << " ability:" << abilityDisplayName(ability);
}
