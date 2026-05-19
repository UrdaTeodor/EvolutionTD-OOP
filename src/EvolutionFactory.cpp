#include "EvolutionFactory.h"
#include "GameException.h"
#include <utility>

using R = Evolution::Rarity;
using A = AbilityEvolution::AbilityType;

//  Mini Stats (~5%)
// cppcheck-suppress unusedFunction // T3
std::unique_ptr<Evolution> makeMiniDamage() {
    return std::make_unique<StatEvolution>("MiniDamage", 20, R::MINI, 0.05f, 0.0f, 0.0f, 0.0f, 0.0f);
}
// cppcheck-suppress unusedFunction // T3
std::unique_ptr<Evolution> makeMiniRange() {
    return std::make_unique<StatEvolution>("MiniRange", 20, R::MINI, 0.0f, 0.05f, 0.0f, 0.0f, 0.0f);
}
// cppcheck-suppress unusedFunction // T3
std::unique_ptr<Evolution> makeMiniAttackSpeed() {
    return std::make_unique<StatEvolution>("MiniAttackSpeed", 20, R::MINI, 0.0f, 0.0f, 0.05f, 0.0f, 0.0f);
}

//Rare (~15%)
// cppcheck-suppress unusedFunction // T3
std::unique_ptr<Evolution> makeRareDamage() {
    return std::make_unique<StatEvolution>("RareDamage", 60, R::RARE, 0.15f, 0.0f, 0.0f, 0.0f, 0.0f);
}
// cppcheck-suppress unusedFunction // T3
std::unique_ptr<Evolution> makeRareRange() {
    return std::make_unique<StatEvolution>("RareRange", 60, R::RARE, 0.0f, 0.15f, 0.0f, 0.0f, 0.0f);
}
// cppcheck-suppress unusedFunction // T3
std::unique_ptr<Evolution> makeRareAttackSpeed() {
    return std::make_unique<StatEvolution>("RareAttackSpeed", 60, R::RARE, 0.0f, 0.0f, 0.15f, 0.0f, 0.0f);
}
// cppcheck-suppress unusedFunction // T3
std::unique_ptr<Evolution> makeRareHP() {
    return std::make_unique<StatEvolution>("RareHP", 60, R::RARE, 0.0f, 0.0f, 0.0f, 0.15f, 0.0f);
}

// Epic
// cppcheck-suppress unusedFunction // T3
std::unique_ptr<Evolution> makeEpicMultiTarget() {
    return std::make_unique<AbilityEvolution>("EpicMultiTarget", 100, R::EPIC, A::MULTI_TARGET);
}
// cppcheck-suppress unusedFunction // T3
std::unique_ptr<Evolution> makeEpicBiggerAura() {
    return std::make_unique<AbilityEvolution>("EpicBiggerAura", 100, R::EPIC, A::BIGGER_AURA);
}
// cppcheck-suppress unusedFunction // T3
std::unique_ptr<Evolution> makeEpicArmored() {
    return std::make_unique<AbilityEvolution>("EpicArmored", 100, R::EPIC, A::ARMORED);
}

//  Legendary
// cppcheck-suppress unusedFunction // T3
std::unique_ptr<Evolution> makeLegendaryDoubleShot() {
    return std::make_unique<AbilityEvolution>("LegendaryDoubleShot", 200, R::LEGENDARY, A::DOUBLE_SHOT);
}
// cppcheck-suppress unusedFunction // T3
std::unique_ptr<Evolution> makeLegendaryFireTrail() {
    return std::make_unique<AbilityEvolution>("LegendaryFireTrail", 200, R::LEGENDARY, A::FIRE_TRAIL);
}
// cppcheck-suppress unusedFunction // T3
std::unique_ptr<Evolution> makeLegendaryKnockback() {
    return std::make_unique<AbilityEvolution>("LegendaryKnockback", 200, R::LEGENDARY, A::KNOCKBACK_EVERY_3);
}
// cppcheck-suppress unusedFunction // T3
std::unique_ptr<Evolution> makeLegendaryReflectiveShield() {
    return std::make_unique<AbilityEvolution>("LegendaryReflectShield", 200, R::LEGENDARY, A::REFLECTIVE_SHIELD);
}
// cppcheck-suppress unusedFunction // T3
std::unique_ptr<Evolution> makeLegendaryMovable() {
    return std::make_unique<AbilityEvolution>("LegendaryMovable", 200, R::LEGENDARY, A::MOVABLE);
}

//  Mythic 
std::unique_ptr<Evolution> craftMythic(
        std::unique_ptr<AbilityEvolution> a,
        std::unique_ptr<AbilityEvolution> b) {
    if (!a || !b)
        throw IncompatibleEvolutionException(
            "craftMythic: cel putin una dintre evolutii e null");
    if (a->getRarity() != Evolution::Rarity::LEGENDARY ||
        b->getRarity() != Evolution::Rarity::LEGENDARY)
        throw IncompatibleEvolutionException(
            "craftMythic: ambele evolutii trebuie sa fie Legendary");
    // arunca IncompatibleEvolutionException daca combo-ul e invalid.

    return std::make_unique<MythicEvolution>(
        "MythicCombo", 500, std::move(a), std::move(b));
}
