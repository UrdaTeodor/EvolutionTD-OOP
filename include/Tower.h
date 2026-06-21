#pragma once
#include <string>
#include <ostream>
#include <vector>
#include <utility>
#include <memory>
#include "Enemy.h"
#include "AbilityType.h"
#include "MythicType.h"
#include "Shot.h"
#include "TowerSpec.h"

class GlobalStatBuffs;   // forward decl, definit in GlobalStatBuffs.h

// Prioritatea de targetare a turnurilor care trag:
//  FIRST  = inamicul cel mai avansat pe drum (default; opreste ce e gata sa intre in baza)
//  CLOSE  = cel mai apropiat de turn (comportamentul vechi)
//  STRONG = HP-ul curent cel mai mare (focus pe tancuri/boss)
//  LAST   = cel mai din spate pe drum (DoT-ul FireTrail are timp maxim sa arda)
enum class TargetingMode { FIRST, CLOSE, STRONG, LAST };

const char*   targetingModeName(TargetingMode m);
TargetingMode targetingModeFromString(const std::string& s);

// Refactor T3 stats vin din TowerSpec (citit din JSON);
class Tower {
    // Id unic per instanta (pastrat la clone/snapshot). Proiectilele in zbor
    // refera turnul-sursa prin id, nu prin pointer (turnul poate fi vandut).
    static int next_instance_id_;
    int instance_id_;

    const TowerSpec* spec_;
    std::string      type_key_;  // ex. "antivirus" - cheia in DataRegistry + GlobalStatBuffs
    int   x_;
    int   y_;
    bool  movable_;
    int   token_investment_ = 0;
    TargetingMode targeting_ = TargetingMode::FIRST;

    // Buff LOCAL de la Honeypot-urile de suport din raza (AMPLIFY/OVERCLOCK).
    // Recalculat de Wave la fiecare tick (nu se salveaza, nu se acumuleaza):
    // muti turnul / vinzi honeypot-ul si buff-ul dispare natural.
    float local_damage_pct_       = 0.0f;
    float local_attack_speed_pct_ = 0.0f;

public:
    Tower(const TowerSpec& spec, std::string type_key, int x, int y);
    virtual ~Tower() = default;



    // out_shots: proiectilele lansate in acest tick (damage-ul se aplica la
    // IMPACT, in Wave::simulate — nu instant). Turnurile fara proiectile
    // (Honeypot/Firewall/Miner) ignora parametrul.
    virtual void update(std::vector<Enemy>& enemies, float deltaTime,
                        const GlobalStatBuffs& buffs,
                        const std::vector<std::pair<int, int>>& path,
                        std::vector<Shot>& out_shots) = 0;
    virtual char getDisplayChar() const = 0;

    // Virtual constructor 
    virtual std::unique_ptr<Tower> clone() const = 0;

    virtual void applyAbility(AbilityType a);

    // Mythic (craft din 2 Legendary). Default arunca IncompatibleEvolution;
    // derivatele care suporta un mythic dau override. Max 1 mythic per turn
    // (anti-stack), verificat de derivate prin mythicBadge().
    virtual void applyMythic(MythicType m);

    // Numele mythic-ului aplicat pe turnul asta, sau nullptr. Folosit de HUD
    // (badge in info panel), de save si de verificarea anti-stack.
    virtual const char* mythicBadge() const { return nullptr; }

    virtual int collectIncome(const GlobalStatBuffs& buffs) const;

    // Ability MOVABLE
    void enableMovable();
    bool isMovable() const { return movable_; }

    // Repozitionare (ability MOVABLE). Validarea celulei o face Game::moveTower.
    void moveTo(int col, int row) { x_ = col; y_ = row; }

    // Pozitia la care se DESENEAZA turnul (poate diferi temporar de celula
    // logica, ex. sarja ShieldedRunner). Default = celula proprie.
    virtual std::pair<float, float> visualPosition() const {
        return { static_cast<float>(x_), static_cast<float>(y_) };
    }

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

    TargetingMode targeting() const         { return targeting_; }
    void setTargeting(TargetingMode m)      { targeting_ = m; }
    void cycleTargeting();

    // Buff-ul local de suport (setat de Wave inainte de update-ul turnurilor).
    void  setLocalBuffs(float damage_pct, float attack_speed_pct) {
        local_damage_pct_       = damage_pct;
        local_attack_speed_pct_ = attack_speed_pct;
    }
    float localDamagePct()      const { return local_damage_pct_; }
    float localAttackSpeedPct() const { return local_attack_speed_pct_; }

    int instanceId() const { return instance_id_; }

    virtual std::vector<AbilityType> getAppliedAbilities() const;

    friend std::ostream& operator<<(std::ostream& os, const Tower& t);

protected:
    // NVI: derivatele suprascriu partea specifica
    virtual void displayDetails(std::ostream& os) const = 0;
};
