#include "Wave.h"
#include "GlobalStatBuffs.h"
#include <iostream>

//Regula celor 3 (cand Wave va contine pointeri la inamici polimorfici)


Wave::Wave(int waveNumber, std::vector<Enemy> enemies)
    : activeEnemies(), pendingEnemies(enemies),
      waveNumber(waveNumber), spawnTimer(0.0f) {}

Wave::Wave(const Wave& other)
    : activeEnemies(other.activeEnemies),
      pendingEnemies(other.pendingEnemies),
      waveNumber(other.waveNumber),
      spawnTimer(other.spawnTimer),
      spawn_interval_(other.spawn_interval_),
      boss_escaped_(other.boss_escaped_) {}


Wave& Wave::operator=(const Wave& other) {
    if (this != &other) {
        activeEnemies   = other.activeEnemies;
        pendingEnemies  = other.pendingEnemies;
        waveNumber      = other.waveNumber;
        spawnTimer      = other.spawnTimer;
        spawn_interval_ = other.spawn_interval_;
        boss_escaped_   = other.boss_escaped_;
    }
    return *this;
}


Wave::~Wave() {}

// towers e vector de unique_ptr<Tower> => apel polimorfic prin tower->update().
int Wave::simulate(std::vector<std::unique_ptr<Tower>>& towers,
                   const std::vector<std::pair<int, int>>& path,
                   float deltaTime,
                   int& moneyEarned,
                   int& killedCount,
                   const GlobalStatBuffs& buffs) {
    moneyEarned = 0;
    killedCount = 0;

    //  Genereaza urmatorul inamic din coada cand timerul expira.
    // Spawn FIFO
    spawnTimer -= deltaTime;
    if (spawnTimer <= 0.0f && !pendingEnemies.empty()) {
        Enemy spawned = pendingEnemies.front();
        pendingEnemies.erase(pendingEnemies.begin());
        spawned.placeAt(static_cast<float>(path[0].second),
                        static_cast<float>(path[0].first));
        activeEnemies.push_back(spawned);
        spawnTimer = spawn_interval_;
    }

    //  Reseteaza incetinirea pe toti inamicii; turnurile Honeypot o vor reaplica
    for (auto& enemy : activeEnemies) {
        enemy.resetSlow();
    }


    for (auto& tower : towers) {
        tower->update(activeEnemies, deltaTime, buffs, path);
    }

    // DoT FIRE_TRAIL: aplicat per-frame pe inamicii cu fire_trail_remaining > 0.
    for (auto& enemy : activeEnemies) {
        if (enemy.isAlive()) enemy.tickFireTrail(deltaTime);
    }

    // Misca fiecare inamic
    for (auto& enemy : activeEnemies) {
        if (enemy.isAlive()) {
            enemy.move(path, deltaTime);
        }
    }

    // inamici ucisi -> bani, inamici scapati -> hp dmg
    int playerDamage = 0;
    std::vector<Enemy> survivors;
    for (const auto& enemy : activeEnemies) {
        if (!enemy.isAlive()) {
            moneyEarned += enemy.getReward();
            killedCount += 1;
        } else if (enemy.hasReachedEnd(path)) {
            int dmg = static_cast<int>(enemy.getCurrentHealth());
            if (enemy.getName() == "ILOVEYOU") {
                boss_escaped_ = true;
                dmg = 99999;   // boss escape = defeat instant indiferent de HP rămas
            }
            playerDamage += dmg;
            std::cout << "  >> " << enemy.getName()
                      << " breached the system! (-" << dmg << " HP)\n";
        } else {
            survivors.push_back(enemy);
        }
    }
    activeEnemies = survivors;

    return playerDamage;
}

void Wave::addEnemy(const Enemy& enemy) {
    pendingEnemies.push_back(enemy);
}

bool Wave::allDefeated() const {
    return pendingEnemies.empty() && activeEnemies.empty();
}

int Wave::activeCount() const {
    return static_cast<int>(activeEnemies.size());
}

const std::vector<Enemy>& Wave::getActiveEnemies() const {
    return activeEnemies;
}

// Friend operator<<: poate accesa direct membrii private ai Wave.
std::ostream& operator<<(std::ostream& os, const Wave& w) {
    os << "Wave " << w.waveNumber
       << " | active:" << w.activeEnemies.size()
       << " | queued:" << w.pendingEnemies.size() << "\n";
    for (const auto& e : w.activeEnemies) {
        os << "  " << e << "\n";  // -> Enemy::operator<< (src/Enemy.cpp)
    }
    return os;
}
