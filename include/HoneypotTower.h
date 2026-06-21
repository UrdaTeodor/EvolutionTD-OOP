#pragma once
#include "Tower.h"
#include <memory>
#include <vector>

class HoneypotTower : public Tower {
    int biggerAuraStacks = 0;
    // Evolutii de SUPORT (v0.4.5): honeypot-ul buff-uieste turnurile din aura.
    int amplifyStacks_   = 0;   // +15% damage per stack
    int overclockStacks_ = 0;   // +25% attack speed per stack

public:
    HoneypotTower(const TowerSpec& spec, int col, int row);

    void update(std::vector<Enemy>& enemies, float deltaTime,
                const GlobalStatBuffs& buffs,
                const std::vector<std::pair<int, int>>& path,
                std::vector<Shot>& out_shots) override;
    char getDisplayChar() const override;
    std::unique_ptr<Tower> clone() const override;

    void applyAbility(AbilityType a) override;
    std::vector<AbilityType> getAppliedAbilities() const override;

    // Raza aurei cu BIGGER_AURA + buff-uri globale aplicate. O singura sursa
    // de adevar pentru slow (update), buff-urile de suport (Wave) si inelul
    // vizual (BoardRenderer).
    float auraRange(const GlobalStatBuffs& buffs) const;

    // Buff-urile oferite turnurilor din aura (0 daca evolutia lipseste).
    float supportDamagePct()      const { return 0.15f * amplifyStacks_; }
    float supportAttackSpeedPct() const { return 0.25f * overclockStacks_; }
    bool  isSupport() const { return amplifyStacks_ > 0 || overclockStacks_ > 0; }
    int   amplifyStacks()   const { return amplifyStacks_; }
    int   overclockStacks() const { return overclockStacks_; }

protected:
    void displayDetails(std::ostream& os) const override;
};
