#include "Evolution.h"
#include <utility>

Evolution::Evolution(std::string name, int cost, Rarity rarity)
    : name(std::move(name)), cost(cost), rarity(rarity) {}

const std::string& Evolution::getName() const { return name; }
int Evolution::getCost() const { return cost; }
Evolution::Rarity Evolution::getRarity() const { return rarity; }

static const char* rarityToStr(Evolution::Rarity r) {
    switch (r) {
        case Evolution::Rarity::MINI:      return "Mini";
        case Evolution::Rarity::RARE:      return "Rare";
        case Evolution::Rarity::EPIC:      return "Epic";
        case Evolution::Rarity::LEGENDARY: return "Legendary";
        case Evolution::Rarity::MYTHIC:    return "Mythic";
    }
    return "?";
}

// displayDetails virtual care e suprascris in fiecare derivata.
std::ostream& operator<<(std::ostream& os, const Evolution& e) {
    os << "[" << rarityToStr(e.rarity) << " " << e.name;
    e.displayDetails(os);
    os << " cost:" << e.cost << "]";
    return os;
}
