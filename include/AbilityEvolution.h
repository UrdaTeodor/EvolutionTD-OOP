#pragma once
#include "Evolution.h"
#include "AbilityType.h"

// Folosit pentru Epic si Legendary Major.
// Tipul abilitatii e ales prin enum-ul AbilityType (definit global in AbilityType.h
// si re-exportat aici ca AbilityEvolution::AbilityType pentru cod existent).
class AbilityEvolution : public Evolution {
public:
    using AbilityType = ::AbilityType;

private:
    AbilityType ability;

public:
    AbilityEvolution(std::string name, int cost, Rarity rarity, AbilityType ability);

    void apply(const EvolutionContext& ctx) override;
    std::unique_ptr<Evolution> clone() const override;

    AbilityType getAbility() const;

    // STATIC: verifica daca 2 abilitati Legendary se pot combina in Mythic.
    // Hardcodat: doar 3 perechi (din 10 posibile) sunt valide.
    static bool canCombine(AbilityType a, AbilityType b);

protected:
    void displayDetails(std::ostream& os) const override;
};
