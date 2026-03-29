#pragma once
#include <vector>
#include <ostream>
#include <utility>
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
    Wave(const Wave& other);             // copie coonstrucor (regula celor 3)
    Wave& operator=(const Wave& other);  // copy operator
    ~Wave();                             // destructor      

    // spawneaza si muta inamicii.
    // returneaza damage un luat de player in acest tick.
    // returneaza banii castigati din inamicii omorati in acest tick prin moneyEarned
    int simulate(std::vector<Tower>& towers,
                 const std::vector<std::pair<int, int>>& path,
                 float deltaTime,
                 int& moneyEarned);

    void addEnemy(const Enemy& enemy);
    bool allDefeated() const;
    int activeCount() const;
    const std::vector<Enemy>& getActiveEnemies() const;

    friend std::ostream& operator<<(std::ostream& os, const Wave& w);
};
