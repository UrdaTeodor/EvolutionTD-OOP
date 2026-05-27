#pragma once
#include <string>
#include <ostream>
#include <vector>
#include <utility>
#include <memory>
#include "Enemy.h"
#include "AbilityType.h"
#include "TowerSpec.h"

class GlobalStatBuffs;   // forward decl, definit in GlobalStatBuffs.h

// Refactor T3 stats vin din TowerSpec (citit din JSON);
class Tower {
    const TowerSpec* spec_;   
    std::string      type_key_;  // ex. "antivirus" - cheia in DataRegistry + GlobalStatBuffs
    int   x_;
    int   y_;
    bool  movable_;
    int   token_investment_ = 0;

public:
    Tower(const TowerSpec& spec, std::string type_key, int x, int y);
    virtual ~Tower() = default;



    virtual void update(std::vector<Enemy>& enemies, float deltaTime,
                        const GlobalStatBuffs& buffs,
                        const std::vector<std::pair<int, int>>& path) = 0;
    virtual char getDisplayChar() const = 0;

    // Virtual constructor 
    virtual std::unique_ptr<Tower> clone() const = 0;

    virtual void applyAbility(AbilityType a);

    virtual int collectIncome(const GlobalStatBuffs& buffs) const;

    // Ability MOVABLe)
    void enableMovable();

    // tracking suma cost token-uri aplicate pe acest instance.
    void recordTokenInvestment(int cost);
    int  getTokenInvestment() const;

    int getX() const;
    int getY() const;
    int getCost() const;
    float getRange() const;                   
    const std::string& getName() const;
    const std::string& getTypeKey() const;
    const TowerSpec& spec() const;

    // Range efectiv cu buff range_pct aplicat. Foloseste in isInRange din derivate.
    float effectiveRange(const GlobalStatBuffs& buffs) const;

    // True daca abilitatea e in TowerSpec.supports_abilities. Folosit de ShopPanel
    // si GameScene ca sa marcheze tower-ele compatibile cu un token (halo verde).
    bool supports(AbilityType ab) const;

    virtual std::vector<AbilityType> getAppliedAbilities() const;

    friend std::ostream& operator<<(std::ostream& os, const Tower& t);

protected:
    // NVI: derivatele suprascriu partea specifica
    virtual void displayDetails(std::ostream& os) const = 0;
};
