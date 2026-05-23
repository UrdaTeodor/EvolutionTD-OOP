#pragma once
#include <string>
#include <ostream>
#include <vector>
#include <memory>
#include "Enemy.h"
#include "AbilityType.h"
#include "TowerSpec.h"

class GlobalStatBuffs;   // forward decl, definit in GlobalStatBuffs.h

// Tower e clasa de baza abstracta pentru toate tipurile de turnuri.
// Refactor T3 stats vin din TowerSpec (citit din JSON);
// instanta retine doar pozitia, evo flags si pointer la spec.
class Tower {
    const TowerSpec* spec_;   
    std::string      type_key_;  // ex. "antivirus" - cheia in DataRegistry + GlobalStatBuffs
    int   x_;
    int   y_;
    bool  movable_;

public:
    Tower(const TowerSpec& spec, std::string type_key, int x, int y);
    virtual ~Tower() = default;

    // Theme-specific: logica de atac. Buffs accumulator pass per call (T3 design).
    virtual void update(std::vector<Enemy>& enemies, float deltaTime,
                        const GlobalStatBuffs& buffs) = 0;
    virtual char getDisplayChar() const = 0;

    // Virtual constructor 
    virtual std::unique_ptr<Tower> clone() const = 0;


    // Default = arunca IncompatibleEvolutionException 
    // Fiecare derivata override pentru ability-urile ei concrete.
    virtual void applyAbility(AbilityType a);

    // BytecoinMinerTower returneaza venit pasiv (cu buff income aplicat).
    virtual int collectIncome(const GlobalStatBuffs& buffs) const;

    // Citita din spec (nu mai e virtual override pe derivata).
    bool requiresPath() const;

    // Ability MOVABLE: setata din applyAbility(MOVABLE)
    void enableMovable();
    bool isMovable() const;

    int getX() const;
    int getY() const;
    int getCost() const;
    float getRange() const;                   
    const std::string& getName() const;
    const std::string& getTypeKey() const;
    const TowerSpec& spec() const;

    // Range efectiv cu buff range_pct aplicat. Foloseste in isInRange din derivate.
    float effectiveRange(const GlobalStatBuffs& buffs) const;

    friend std::ostream& operator<<(std::ostream& os, const Tower& t);

protected:
    // NVI: derivatele suprascriu partea specifica
    virtual void displayDetails(std::ostream& os) const = 0;
};
