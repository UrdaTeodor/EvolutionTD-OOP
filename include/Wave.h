#pragma once
#include <vector>
#include <ostream>
#include <utility>
#include <memory>
#include "Enemy.h"
#include "Tower.h"

class GlobalStatBuffs;   // forward decl

class Wave {
    std::vector<Enemy> activeEnemies;
    std::vector<Enemy> pendingEnemies;
    int waveNumber;
    float spawnTimer;
    float spawn_interval_ = 2.0f;
    bool  boss_escaped_   = false;   // true daca un boss a ajuns la final de path (defeat)

public:

    Wave(int waveNumber, std::vector<Enemy> enemies);

    void setSpawnInterval(float seconds) { spawn_interval_ = seconds; }
    bool bossEscaped() const { return boss_escaped_; }


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
    int activeCount() const;                    // cati inamici sunt pe ecran
    const std::vector<Enemy>& getActiveEnemies() const;

    friend std::ostream& operator<<(std::ostream& os, const Wave& w);
};
