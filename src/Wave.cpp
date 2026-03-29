#include "Wave.h"
#include <iostream>

//Regula celor 3 (cand Wave va contine pointeri la inamici polimorfici)

Wave::Wave(int waveNumber, std::vector<Enemy> enemies)
    : activeEnemies(), pendingEnemies(enemies),
      waveNumber(waveNumber), spawnTimer(0.0f) {}

Wave::Wave(const Wave& other)
    : activeEnemies(other.activeEnemies),
      pendingEnemies(other.pendingEnemies),
      waveNumber(other.waveNumber),
      spawnTimer(other.spawnTimer) {}

Wave& Wave::operator=(const Wave& other) {
    if (this != &other) {
        activeEnemies  = other.activeEnemies;
        pendingEnemies = other.pendingEnemies;
        waveNumber     = other.waveNumber;
        spawnTimer     = other.spawnTimer;
    }
    return *this;
}

Wave::~Wave() {
    //poate fi folosit in viitor pentru curatare (ex. efecte vizuale, sunet)
}

//simuleaza un tick al valului

int Wave::simulate(std::vector<Tower>& towers,
                   const std::vector<std::pair<int, int>>& path,
                   float deltaTime,
                   int& moneyEarned) {
    moneyEarned = 0;

    // 1. Genereaza urmatorul inamic din coada cand timerul expira
    spawnTimer -= deltaTime;
    if (spawnTimer <= 0.0f && !pendingEnemies.empty()) {
        Enemy spawned = pendingEnemies.back();
        pendingEnemies.pop_back();
        spawned.placeAt(static_cast<float>(path[0].second), 
                        static_cast<float>(path[0].first));  
        activeEnemies.push_back(spawned);
        spawnTimer = SPAWN_INTERVAL;
    }

    // 2. Reseteaza incetinirea pe toti inamicii; turnurile Honeypot o vor reaplica
    for (auto& enemy : activeEnemies) {
        enemy.resetSlow();
    }

    // 3. Fiecare turn actioneaza
    for (auto& tower : towers) {
        tower.update(activeEnemies, deltaTime);
    }

    // 4. Misca fiecare inamic
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
        } else if (enemy.hasReachedEnd(path)) {
            playerDamage += static_cast<int>(enemy.getCurrentHealth());
            std::cout << "  >> " << enemy.getName()
                      << " breached the system! (-" << static_cast<int>(enemy.getCurrentHealth()) << " HP)\n";
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

std::ostream& operator<<(std::ostream& os, const Wave& w) {
    os << "Wave " << w.waveNumber
       << " | active:" << w.activeEnemies.size()
       << " | queued:" << w.pendingEnemies.size() << "\n";
    for (const auto& e : w.activeEnemies) {
        os << "  " << e << "\n";
    }
    return os;
}
