#pragma once
#include <vector>
#include <ostream>
#include <utility>
#include <memory>
#include "Enemy.h"
#include "Tower.h"

class Wave {
    std::vector<Enemy> activeEnemies;   
    std::vector<Enemy> pendingEnemies; 
    int waveNumber;
    float spawnTimer;                   
    static constexpr float SPAWN_INTERVAL = 2.0f;

public:

    Wave(int waveNumber, std::vector<Enemy> enemies);


    Wave(const Wave& other);              // constructor de copiere
    Wave& operator=(const Wave& other);   // operator de atribuire
    ~Wave();                              // destructor

    // simuleaza un tick
    // Returneaza damage-ul incasat de jucator in acest tick.
    // moneyEarned e parametru OUT (referinta): functia scrie banii castigati acolo.
    // towers e vector de unique_ptr<Tower> (pointeri la baza)
    // tower->update(...) polimorfic (cerinta T2).
    int simulate(std::vector<std::unique_ptr<Tower>>& towers,
                 const std::vector<std::pair<int, int>>& path,
                 float deltaTime,
                 int& moneyEarned);

    void addEnemy(const Enemy& enemy);          // adauga in pendingEnemies
    bool allDefeated() const;                   // true daca nu mai e nimic de procesat
    int activeCount() const;                    // cati inamici sunt pe ecran
    const std::vector<Enemy>& getActiveEnemies() const;

    friend std::ostream& operator<<(std::ostream& os, const Wave& w);
};
