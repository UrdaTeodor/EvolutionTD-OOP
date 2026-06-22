#include "StatEvolution.h"
#include "GlobalStatBuffs.h"
#include "GameException.h"
#include <utility>

StatEvolution::StatEvolution(std::string name, int cost, Rarity rarity,
                             float damageBoost, float rangeBoost, float attackSpeedBoost,
                             float hpBoost, float regenBoost)
    : Evolution(std::move(name), cost, rarity),
      damageBoostPct(damageBoost),
      rangeBoostPct(rangeBoost),
      attackSpeedBoostPct(attackSpeedBoost),
      hpBoostPct(hpBoost),
      regenBoostPct(regenBoost) {}

// T3 refactor, Aplica direct pe GlobalStatBuffs pentru tower
// Tower-ele citesc apoi buff-urile din accumulator cand calculeaza stats efective.
void StatEvolution::apply(const EvolutionContext& ctx) {
    if (!ctx.buffs) {
        throw IncompatibleEvolutionException(
            "StatEvolution.apply: buffs lipseste din EvolutionContext");
    }
    if (ctx.target_type_key.empty()) {
        throw IncompatibleEvolutionException(
            "StatEvolution.apply: target_type_key gol");
    }
    const std::string& k = ctx.target_type_key;
    ctx.buffs->add(k, "damage_pct",       damageBoostPct);
    ctx.buffs->add(k, "range_pct",        rangeBoostPct);
    ctx.buffs->add(k, "attack_speed_pct", attackSpeedBoostPct);
    ctx.buffs->add(k, "max_hp_pct",       hpBoostPct);
    ctx.buffs->add(k, "regen_pct",        regenBoostPct);
}

std::unique_ptr<Evolution> StatEvolution::clone() const {
    return std::make_unique<StatEvolution>(*this);
}

void StatEvolution::displayDetails(std::ostream& os) const {
    os << " dmg+" << damageBoostPct*100 << "%"
       << " rng+" << rangeBoostPct*100 << "%"
       << " aspd+" << attackSpeedBoostPct*100 << "%";
    if (hpBoostPct    != 0.0f) os << " hp+"   << hpBoostPct*100   << "%";
    if (regenBoostPct != 0.0f) os << " regen+" << regenBoostPct*100 << "%";
}
