#pragma once
#include <string>
#include <ostream>
#include <memory>

//apply(Tower&) ia referinta, nu trb include complet aici
class Tower;


// Are 3 derivate: StatEvolution, AbilityEvolution, MythicEvolution.
// Aplicat pe un turn prin apply()
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

    // aplica efectul evo
    virtual void apply(Tower& target) = 0;

    // virtual constructor (clone):copierea polimorfica
    // (folosit la inventar + la MythicEvolution care detine 2 surse)
    virtual std::unique_ptr<Evolution> clone() const = 0;

    const std::string& getName() const;
    int getCost() const;
    Rarity getRarity() const;

    friend std::ostream& operator<<(std::ostream& os, const Evolution& e);

protected:
    // NVI pattern: operator<< cheama displayDetails (virtual)
    virtual void displayDetails(std::ostream& os) const = 0;
};
