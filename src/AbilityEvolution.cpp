#include "AbilityEvolution.h"
#include "AntivirusTower.h"
#include "AdblockerTower.h"
#include "FirewallTower.h"
#include "HoneypotTower.h"
#include "GameException.h"
#include <algorithm>   // std::minmax
#include <utility>

AbilityEvolution::AbilityEvolution(std::string name, int cost, Rarity rarity, AbilityType ability)
    : Evolution(std::move(name), cost, rarity), ability(ability) {}


// switch pe enum decide CE abilitate; dynamic_cast verifica DACA turnul o suporta.
// Daca turnul NU suporta abilitatea -> IncompatibleEvolutionException
void AbilityEvolution::apply(Tower& target) {
    switch (ability) {
        // ---- Epic ----
        case AbilityType::MULTI_TARGET:
            if (auto* anti = dynamic_cast<AntivirusTower*>(&target))      anti->enableMultiTarget();
            else if (auto* adb = dynamic_cast<AdblockerTower*>(&target)) adb->enableMultiTarget();
            else throw IncompatibleEvolutionException(
                "MultiTarget: doar Antivirus si Adblocker accepta aceasta abilitate");
            break;
        case AbilityType::BIGGER_AURA:
            if (auto* hp = dynamic_cast<HoneypotTower*>(&target))        hp->enableBiggerAura();
            else throw IncompatibleEvolutionException(
                "BiggerAura: doar Honeypot accepta aceasta abilitate");
            break;
        case AbilityType::ARMORED:
            if (auto* fw = dynamic_cast<FirewallTower*>(&target))        fw->enableArmored();
            else throw IncompatibleEvolutionException(
                "Armored: doar Firewall accepta aceasta abilitate");
            break;

        // ---- Legendary ----
        case AbilityType::DOUBLE_SHOT:
            if (auto* anti = dynamic_cast<AntivirusTower*>(&target))      anti->enableDoubleShot();
            else if (auto* adb = dynamic_cast<AdblockerTower*>(&target)) adb->enableDoubleShot();
            else throw IncompatibleEvolutionException(
                "DoubleShot: doar Antivirus si Adblocker accepta aceasta abilitate");
            break;
        case AbilityType::FIRE_TRAIL:
            if (auto* anti = dynamic_cast<AntivirusTower*>(&target))      anti->enableFireTrail();
            else if (auto* adb = dynamic_cast<AdblockerTower*>(&target)) adb->enableFireTrail();
            else throw IncompatibleEvolutionException(
                "FireTrail: doar Antivirus si Adblocker accepta aceasta abilitate");
            break;
        case AbilityType::KNOCKBACK_EVERY_3:
            if (auto* anti = dynamic_cast<AntivirusTower*>(&target))      anti->enableKnockback(3);
            else if (auto* adb = dynamic_cast<AdblockerTower*>(&target)) adb->enableKnockback(3);
            else throw IncompatibleEvolutionException(
                "Knockback: doar Antivirus si Adblocker accepta aceasta abilitate");
            break;
        case AbilityType::REFLECTIVE_SHIELD:
            if (auto* fw = dynamic_cast<FirewallTower*>(&target))        fw->enableReflectiveShield();
            else throw IncompatibleEvolutionException(
                "ReflectiveShield: doar Firewall accepta aceasta abilitate");
            break;
        case AbilityType::MOVABLE:
            //orice turn poate fi movable
            target.enableMovable();
            break;
    }
}

std::unique_ptr<Evolution> AbilityEvolution::clone() const {
    return std::make_unique<AbilityEvolution>(*this);
}

AbilityEvolution::AbilityType AbilityEvolution::getAbility() const { return ability; }

// 3 combinatii hardcodate. Restul perechilor returneaza false.
// std::minmax normalizeaza ordinea ca sa nu mai scriem (a,b) si (b,a) separat
// cppcheck-suppress unusedFunction // T3
bool AbilityEvolution::canCombine(AbilityType a, AbilityType b) {
    if (a == b) return false;  
    auto p = std::minmax(a, b);
    auto x = p.first;
    auto y = p.second;

    // PHOENIX_BARRAGE = DOUBLE_SHOT + FIRE_TRAIL
    if (x == AbilityType::DOUBLE_SHOT && y == AbilityType::FIRE_TRAIL) return true;
    // ROVING_BRUISER = KNOCKBACK_EVERY_3 + MOVABLE
    if (x == AbilityType::KNOCKBACK_EVERY_3 && y == AbilityType::MOVABLE) return true;
    // SHIELDED_RUNNER = REFLECTIVE_SHIELD + MOVABLE
    if (x == AbilityType::REFLECTIVE_SHIELD && y == AbilityType::MOVABLE) return true;

    return false;
}

static const char* abilityToStr(AbilityEvolution::AbilityType a) {
    using A = AbilityEvolution::AbilityType;
    switch (a) {
        case A::MULTI_TARGET:      return "MultiTarget";
        case A::BIGGER_AURA:       return "BiggerAura";
        case A::ARMORED:           return "Armored";
        case A::DOUBLE_SHOT:       return "DoubleShot";
        case A::FIRE_TRAIL:        return "FireTrail";
        case A::KNOCKBACK_EVERY_3: return "Knockback/3";
        case A::REFLECTIVE_SHIELD: return "ReflectShield";
        case A::MOVABLE:           return "Movable";
    }
    return "?";
}

void AbilityEvolution::displayDetails(std::ostream& os) const {
    os << " ability:" << abilityToStr(ability);
}
