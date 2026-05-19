#pragma once
#include "Evolution.h"

// StatEvolution: aplica boost-uri numerice.
// Folosit pentru Mini si Rare 
// Foloseste dynamic_cast in apply() pentru a chema buff-uri specifice tipului de turn.
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

    void apply(Tower& target) override;
    std::unique_ptr<Evolution> clone() const override;

protected:
    void displayDetails(std::ostream& os) const override;
};
