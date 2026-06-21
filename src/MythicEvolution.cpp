#include "MythicEvolution.h"
#include "GameException.h"
#include "EvolutionContext.h"
#include "Tower.h"
#include <algorithm>
#include <utility>

MythicEvolution::MythicEvolution(std::string name, int cost,
                                 std::unique_ptr<AbilityEvolution> a,
                                 std::unique_ptr<AbilityEvolution> b)
    : Evolution(std::move(name), cost, Rarity::MYTHIC),
      source1(std::move(a)),
      source2(std::move(b)),
      mythicType(deriveType(source1->getAbility(), source2->getAbility())) {}

// Copy constructor
MythicEvolution::MythicEvolution(const MythicEvolution& other)
    : Evolution(other),
      source1(std::make_unique<AbilityEvolution>(*other.source1)),
      source2(std::make_unique<AbilityEvolution>(*other.source2)),
      mythicType(other.mythicType) {}

// Operator= COPY-AND-SWAP:
// parametrul 'other' e primit by value -> compilatorul a apelat deja copy constructor
// swap(*this, other) interschimba intern continutul
// cand 'other' iese din scope, ii destrugem vechea valoare (ce era in *this)
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

void MythicEvolution::apply(const EvolutionContext& ctx) {
    if (!ctx.target_tower) {
        throw IncompatibleEvolutionException("MythicEvolution.apply: invalid target");
    }
    // Anti-stack: verificam INAINTE sa aplicam sursele, altfel un turn care are
    // deja un Mythic ar primi abilitatile-sursa si abia apoi exceptia.
    if (ctx.target_tower->mythicBadge() != nullptr) {
        throw IncompatibleEvolutionException("Turnul are deja un Mythic (max 1 per turn).");
    }

    source1->apply(ctx);
    source2->apply(ctx);

    // Efectul unic al mythic-ului (PhoenixBarrage / RovingBruiser / ShieldedRunner),
    // implementat in derivata de Tower corespunzatoare.
    ctx.target_tower->applyMythic(mythicType);
}

std::unique_ptr<Evolution> MythicEvolution::clone() const {
    return std::make_unique<MythicEvolution>(*this);
}


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
