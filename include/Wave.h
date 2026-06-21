#pragma once
#include <vector>
#include <ostream>
#include <utility>
#include <memory>
#include "Enemy.h"
#include "Shot.h"
#include "Tower.h"
#include "VisualEvent.h"

class GlobalStatBuffs;   // forward decl

class Wave {
    std::vector<Enemy> activeEnemies;
    std::vector<Enemy> pendingEnemies;
    // Proiectile in zbor (homing). Damage-ul se aplica la impact, in tickShots.
    std::vector<Shot> shots_;
    // Ce s-a intamplat in tick-urile de la ultimul takeVisualEvents (trageri,
    // impacturi, morti). Doar date pentru EffectsLayer; logica nu le citeste.
    std::vector<VisualEvent> visual_events_;
    int waveNumber;
    float spawnTimer;
    float spawn_interval_ = 2.0f;
    bool  boss_escaped_   = false;   // true daca boss-ul SCRIPTAT a ajuns la final (defeat)
    // Doar boss-ul scriptat (valul final al campaniei) da defeat instant la
    // scapare. ILOVEYOU-urile spawnate de Director in endless sunt inamici
    // obisnuiti: fac leak normal din EnemySpec.
    bool  final_boss_wave_ = false;

    // Misca proiectilele spre tintele lor si aplica impacturile.
    void tickShots(std::vector<std::unique_ptr<Tower>>& towers,
                   const std::vector<std::pair<int, int>>& path,
                   float deltaTime);

public:

    Wave(int waveNumber, std::vector<Enemy> enemies);

    // cppcheck-suppress unusedFunction
    void setSpawnInterval(float seconds) { spawn_interval_ = seconds; }
    // cppcheck-suppress unusedFunction
    bool bossEscaped() const { return boss_escaped_; }
    void setFinalBossWave(bool v) { final_boss_wave_ = v; }


    Wave(const Wave& other);              // constructor de copiere
    Wave& operator=(const Wave& other);   // operator de atribuire
    ~Wave();                              // destructor

    // simuleaza un tick
    // Returneaza damage-ul incasat de jucator in acest tick.
    // moneyEarned + killedCount sunt parametri OUT (referinta).
    // towers e vector de unique_ptr<Tower> (pointeri la baza)
    // tower->update(...) polimorfic
    int simulate(std::vector<std::unique_ptr<Tower>>& towers,
                 const std::vector<std::pair<int, int>>& path,
                 float deltaTime,
                 int& moneyEarned,
                 int& killedCount,
                 const GlobalStatBuffs& buffs);

    void addEnemy(const Enemy& enemy);          // adauga in pendingEnemies
    bool allDefeated() const;                   // true daca nu mai e nimic de procesat
    const std::vector<Enemy>& getActiveEnemies() const;
    // Proiectilele in zbor — EffectsLayer le deseneaza direct (vizual = adevar).
    const std::vector<Shot>& getShots() const { return shots_; }

    // Goleste si returneaza evenimentele vizuale acumulate (o data pe frame).
    std::vector<VisualEvent> takeVisualEvents();

    friend std::ostream& operator<<(std::ostream& os, const Wave& w);
};
