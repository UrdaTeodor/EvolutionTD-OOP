#pragma once
#include "Evolution.h"

// StatEvolution: aplica buffuri minore pe TowerType (T3 refactor).
// Folosit pentru Mini si Rare.
// Inainte foloseam dynamic_cast<...Tower*> pe target; acum aplica pe GlobalStatBuffs
// pentru target_type_key din context, fara dynamic_cast.
class StatEvolution : public Evolution {
    float damageBoostPct;       // % buff la damage (Antivirus/Adblocker)
    float rangeBoostPct;        // % buff la raza (toate turnurile - firewall))
    float attackSpeedBoostPct;  // % buff la attack speed (Antivirus/Adblocker)
    float hpBoostPct;           // % buff la maxHP (Firewall)
    float regenBoostPct;        // % buff la regen rate (Firewall)

public:
    StatEvolution(std::string name, int cost, Rarity rarity,
                  float damageBoost, float rangeBoost, float attackSpeedBoost,
                  float hpBoost, float regenBoost);

    void apply(const EvolutionContext& ctx) override;
    std::unique_ptr<Evolution> clone() const override;

protected:
    void displayDetails(std::ostream& os) const override;
};
