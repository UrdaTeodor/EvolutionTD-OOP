#pragma once
#include <string>
#include "AbilityType.h"
#include "Evolution.h"

struct EvolutionToken {
    std::string name;
    AbilityType ability;
    Evolution::Rarity rarity;
    int cost = 0;
};
