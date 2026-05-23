#pragma once
#include <string>

class Tower;
class GlobalStatBuffs;

// Context pasat la Evolution::apply(). Contine ce poate avea nevoie o evolutie.
// StatEvolution foloseste buffs + target_type_key (aplica global pe tower type).
// AbilityEvolution foloseste target_tower (aplica pe instanta aleasa de user prin token).
// MythicEvolution combina cele 2 (aplica 2 ability-uri pe target_tower).
//
// Permite virtual apply uniform pe Evolution fara dynamic_cast la caller.
struct EvolutionContext {
    GlobalStatBuffs* buffs           = nullptr;   // nu-i null la apply 
    std::string      target_type_key;             // pt StatEvolution
    Tower*           target_tower    = nullptr;   // pt AbilityEvolution (nullable)
};
