#pragma once
#include "Evolution.h"
#include "AbilityType.h"

// Folosit pentru Epic si Legendary Major.
// Tipul abilitatii e ales prin enum-ul AbilityType (definit global in AbilityType.h
// aici ca AbilityEvolution::AbilityType.
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

protected:
    void displayDetails(std::ostream& os) const override;
};
