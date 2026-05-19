#pragma once
#include "Evolution.h"
#include "StatEvolution.h"
#include "AbilityEvolution.h"
#include "MythicEvolution.h"

// Factory functions: placeholdere pentru evolutii specifice.
// Pot fi modificate/extinse pe parcurs.

// ---- Mini Stats (~5%) ----
std::unique_ptr<Evolution> makeMiniDamage();
std::unique_ptr<Evolution> makeMiniRange();
std::unique_ptr<Evolution> makeMiniAttackSpeed();

// ---- Rare Major Stats (~15%) ----
std::unique_ptr<Evolution> makeRareDamage();
std::unique_ptr<Evolution> makeRareRange();
std::unique_ptr<Evolution> makeRareAttackSpeed();
std::unique_ptr<Evolution> makeRareHP();

// ---- Epic Major Abilities ----
std::unique_ptr<Evolution> makeEpicMultiTarget();
std::unique_ptr<Evolution> makeEpicBiggerAura();
std::unique_ptr<Evolution> makeEpicArmored();

// ---- Legendary Major Abilities (5 piese; 3 combinatii valide) ----
std::unique_ptr<Evolution> makeLegendaryDoubleShot();
std::unique_ptr<Evolution> makeLegendaryFireTrail();
std::unique_ptr<Evolution> makeLegendaryKnockback();
std::unique_ptr<Evolution> makeLegendaryReflectiveShield();
std::unique_ptr<Evolution> makeLegendaryMovable();

// ---- Helper pentru crafting Mythic ----
// Returneaza nullptr daca:
//   - oricare e null
//   - cele 2 nu sunt amandoua Legendary
//   - perechea nu e una din cele 3 hardcodate
// Consuma cele 2 evolutii (le muta) pe success.
std::unique_ptr<Evolution> craftMythic(
    std::unique_ptr<AbilityEvolution> a,
    std::unique_ptr<AbilityEvolution> b);
