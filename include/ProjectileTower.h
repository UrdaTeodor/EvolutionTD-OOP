#pragma once
#include "Tower.h"
#include <string>
#include <utility>
#include <vector>

// Baza comuna pentru turnurile care trag proiectile (Antivirus, Adblocker).
// Inainte, update()/applyAbility()/stacks erau duplicate 1:1 in cele doua clase;
// acum logica de tras + targeting (FIRST/CLOSE/STRONG) traieste o singura data aici.
// Diferentele dintre Antivirus si Adblocker raman pur de date (TowerSpec).
class ProjectileTower : public Tower {
    int doubleShotStacks  = 0;
    int fireTrailStacks   = 0;
    int multiTargetStacks = 0;
    int splashStacks      = 0;   // BlastWave: AoE la impact
    int knockbackInterval = 0;
    int knockbackCells    = 1;
    int shotCounter       = 0;
    float attackCooldown  = 0.0f;

    // Mythics (max unul per turn, verificat in applyMythic):
    // PhoenixBarrage: kill-urile pe inamici care ard dau embers; la 10 embers,
    // furtuna de foc aprinde TOTI inamicii de pe harta.
    bool phoenix_   = false;
    int  embers_    = 0;
    // RovingBruiser: knockback-ul devine unda de soc (AoE) si aplica
    // vulnerabilitate (+25% damage de la toate turnurile, refresh nu stack).
    bool shockwave_ = false;

    // Anti-cheese knockback: max KNOCK_CAP_PER_SEC procs pe secunda per turn;
    // excesul devine slow 50% / 1s (decizie user, nu mai produce stall infinit).
    static constexpr int KNOCK_CAP_PER_SEC = 2;
    float knock_window_ = 0.0f;
    int   knock_procs_  = 0;

    // Masca de stiluri vizuale pentru proiectile (Shot.style_mask), dupa regula
    // de tier: mythic-ul ascunde legendarele-ingredient, restul raman.
    int shotStyleMask() const;

    bool isInRange(const Enemy& enemy, float effectiveRange) const;

    // Scor de prioritate dupa TargetingMode (mai mare = tinta preferata).
    float targetScore(const Enemy& enemy,
                      const std::vector<std::pair<int, int>>& path) const;

    // Indecsii inamicilor vii din range, sortati best-first dupa targetScore.
    std::vector<size_t> targetIndices(const std::vector<Enemy>& enemies, float range,
                                      const std::vector<std::pair<int, int>>& path,
                                      int maxCount) const;

protected:
    ProjectileTower(const TowerSpec& spec, std::string type_key, int col, int row);

public:
    void update(std::vector<Enemy>& enemies, float deltaTime,
                const GlobalStatBuffs& buffs,
                const std::vector<std::pair<int, int>>& path,
                std::vector<Shot>& out_shots) override;

    // Wave anunta turnul cand un proiectil Phoenix a ucis un inamic care ardea.
    void addEmber() { ++embers_; }
    // EffectsLayer: orbita de embers pe turn (counter vizibil de firestorm).
    int  emberCount() const { return embers_; }

    void applyAbility(AbilityType a) override;
    void applyMythic(MythicType m) override;
    const char* mythicBadge() const override;
    std::vector<AbilityType> getAppliedAbilities() const override;

    // Apelat din AbilityEvolution::apply / restore din save (dynamic_cast la baza asta).
    void setKnockbackInterval(int N);


protected:
    void displayDetails(std::ostream& os) const override;
};
