#pragma once
#include "Evolution.h"


class StatEvolution : public Evolution {
    float damageBoostPct;       // % buff 
    float rangeBoostPct;        
    float attackSpeedBoostPct;  
    float hpBoostPct;           
    float regenBoostPct;        

public:
    StatEvolution(std::string name, int cost, Rarity rarity,
                  float damageBoost, float rangeBoost, float attackSpeedBoost,
                  float hpBoost, float regenBoost);

    void apply(const EvolutionContext& ctx) override;
    std::unique_ptr<Evolution> clone() const override;

protected:
    void displayDetails(std::ostream& os) const override;
};
