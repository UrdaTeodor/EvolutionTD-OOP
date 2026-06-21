#pragma once
#include <string>
#include "AbilityType.h"
#include "Evolution.h"

// Token de evolutie in inventar (max 3, FIFO).
// Token normal: o singura abilitate (Major ABILITY cumparat din shop).
// Mythic Token (universal, castigat din evenimentul de loterie): se aplica pe
// orice turn care are deja 2 Legendare compatibile (o reteta mythic) aplicate;
// campul `ability` e ignorat pentru el.
struct EvolutionToken {
    std::string name;
    AbilityType ability;
    Evolution::Rarity rarity;
    int cost = 0;

    bool is_wildcard = false;   // Mythic Token universal
};
