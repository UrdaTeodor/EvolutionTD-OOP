#pragma once
#include <string>
#include <ostream>
#include <memory>
#include "EvolutionContext.h"

// Are 3 derivate: StatEvolution, AbilityEvolution, MythicEvolution.
// T3 refactor: apply primeste EvolutionContext (buffs + opt tower target),
// nu mai e dependent direct de Tower.
class Evolution {
public:
    // Rarity = tag pentru shop/inventar
    // Mecanismul e clasa: Mini & Rare = StatEvolution, Epic & Legendary = AbilityEvolution.,mythic = combinatie de 2 evolutii Legendary (verificate la craftMythic)
    enum class Rarity { MINI, RARE, EPIC, LEGENDARY, MYTHIC };

private:
    std::string name;
    int cost;
    Rarity rarity;

public:
    Evolution(std::string name, int cost, Rarity rarity);
    virtual ~Evolution() = default;

    // T3: aplica efectul prin context. StatEvolution prin buffs+target_type_key,
    // AbilityEvolution prin target_tower
    virtual void apply(const EvolutionContext& ctx) = 0;

    // virtual constructor ,copierea polimorfica
    // (folosit la inventar + la MythicEvolution care are2 surse)
    virtual std::unique_ptr<Evolution> clone() const = 0;

    const std::string& getName() const;
    int getCost() const;

    friend std::ostream& operator<<(std::ostream& os, const Evolution& e);

protected:
    // NVI pattern: operator<< cheama displayDetails (virtual)
    virtual void displayDetails(std::ostream& os) const = 0;
};
