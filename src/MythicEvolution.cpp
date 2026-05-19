#include "MythicEvolution.h"
#include "AntivirusTower.h"
#include "AdblockerTower.h"
#include "FirewallTower.h"
#include "HoneypotTower.h"
#include "GameException.h"
#include <algorithm>
#include <utility>

MythicEvolution::MythicEvolution(std::string name, int cost,
                                 std::unique_ptr<AbilityEvolution> a,
                                 std::unique_ptr<AbilityEvolution> b)
    : Evolution(std::move(name), cost, Rarity::MYTHIC),
      source1(std::move(a)),
      source2(std::move(b)),
      mythicType(deriveType(source1->getAbility(), source2->getAbility())) {}

// Copy constructor: deep-copy al surselor (clone polimorfic).
// unique_ptr nu se copiaza singur, deci trebuie facem noi instante.
MythicEvolution::MythicEvolution(const MythicEvolution& other)
    : Evolution(other),
      source1(std::make_unique<AbilityEvolution>(*other.source1)),
      source2(std::make_unique<AbilityEvolution>(*other.source2)),
      mythicType(other.mythicType) {}

// Operator= COPY-AND-SWAP:
// - parametrul 'other' e primit BY VALUE -> compilatorul a apelat deja copy constructor
// - swap(*this, other) interschimba intern continutul
// - cand 'other' iese din scope, ii destrugem vechea valoare (ce era in *this)
MythicEvolution& MythicEvolution::operator=(MythicEvolution other) {
    swap(*this, other);
    return *this;
}

// Friend swap (apelat din operator= si util in general)
void swap(MythicEvolution& a, MythicEvolution& b) noexcept {
    using std::swap;
    swap(a.source1, b.source1);
    swap(a.source2, b.source2);
    swap(a.mythicType, b.mythicType);
}

void MythicEvolution::apply(Tower& target) {
    source1->apply(target);
    source2->apply(target);

    // 2. efect mythic specific (placeholder)
    switch (mythicType) {
        case MythicType::PHOENIX_BARRAGE:
            // TODO: 
            break;
        case MythicType::ROVING_BRUISER:
            // TODO: 
            break;
        case MythicType::SHIELDED_RUNNER:
            // TODO: 
            break;
    }
}

std::unique_ptr<Evolution> MythicEvolution::clone() const {
    return std::make_unique<MythicEvolution>(*this);
}

// cppcheck-suppress unusedFunction // T3
MythicEvolution::MythicType MythicEvolution::getMythicType() const { return mythicType; }


MythicEvolution::MythicType MythicEvolution::deriveType(
        AbilityEvolution::AbilityType a, AbilityEvolution::AbilityType b) {
    auto p = std::minmax(a, b);
    if (p.first == AbilityEvolution::AbilityType::DOUBLE_SHOT &&
        p.second == AbilityEvolution::AbilityType::FIRE_TRAIL)
        return MythicType::PHOENIX_BARRAGE;
    if (p.first == AbilityEvolution::AbilityType::KNOCKBACK_EVERY_3 &&
        p.second == AbilityEvolution::AbilityType::MOVABLE)
        return MythicType::ROVING_BRUISER;
    if (p.first == AbilityEvolution::AbilityType::REFLECTIVE_SHIELD &&
        p.second == AbilityEvolution::AbilityType::MOVABLE)
        return MythicType::SHIELDED_RUNNER;
    throw IncompatibleEvolutionException(
        "MythicEvolution: combinatie de abilitati invalida (nu se afla in cele 3 hardcodate)");
}

static const char* mythicToStr(MythicEvolution::MythicType m) {
    using M = MythicEvolution::MythicType;
    switch (m) {
        case M::PHOENIX_BARRAGE: return "PhoenixBarrage";
        case M::ROVING_BRUISER:  return "RovingBruiser";
        case M::SHIELDED_RUNNER: return "ShieldedRunner";
    }
    return "?";
}

void MythicEvolution::displayDetails(std::ostream& os) const {
    os << " type:" << mythicToStr(mythicType);
}
