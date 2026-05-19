#include "StatEvolution.h"
#include "AntivirusTower.h"
#include "AdblockerTower.h"
#include "FirewallTower.h"
#include "HoneypotTower.h" 
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

// Aplica buff-urile pe turn. dynamic_cast pentru cele specifice tipului.
void StatEvolution::apply(Tower& target) {

    target.buffRange(rangeBoostPct);

    // damage / attackSpeed: doar Antivirus si Adblocker au aceste stats
    if (auto* anti = dynamic_cast<AntivirusTower*>(&target)) {
        anti->buffDamage(damageBoostPct);
        anti->buffAttackSpeed(attackSpeedBoostPct);
    } else if (auto* adb = dynamic_cast<AdblockerTower*>(&target)) {
        adb->buffDamage(damageBoostPct);
        adb->buffAttackSpeed(attackSpeedBoostPct);
    } else if (auto* fw = dynamic_cast<FirewallTower*>(&target)) {
        fw->buffMaxHP(hpBoostPct);
        fw->buffRegenRate(regenBoostPct);
    }
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
