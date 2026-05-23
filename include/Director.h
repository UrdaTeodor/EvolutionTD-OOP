#pragma once
#include <random>
#include <vector>
#include <string>
#include "EnemySpec.h"
#include "WeightedTable.hpp"

// Directorul tine un pool de inamici deblocati progressiv prin
// unlocks din WaveSpec. La build wave, primeste un budget si cheltuie pe spawnuri
// random ponderate.
//
// Game ii dA referinta la RNG-ul propriu (seedable).
class Director {
    WeightedTable<EnemySpec> pool_;
    std::mt19937*            rng_;     // non-owning; (seed)
    std::vector<std::string> unlocked_keys_;   // tracking 

public:
    explicit Director(std::mt19937& rng);

    // Necesar dupa copy/swap pe Game, rng_ pointer-ul ramane si trb corectat
    void setRng(std::mt19937& rng) { rng_ = &rng; }

    // Adauga un EnemySpec in pool (apelat la endWave dupa ce wave_N.unlocks_after se aplica).
    // weight default 1.0. 
    void unlock(const std::string& key, const EnemySpec& spec, float weight = 1.0f);

    // Genereaza o lista de EnemySpec spawnari pentru un wave,duoa buget.
    // e greedy, sample uniform din pool, push daca cost <= budget remaining, decrement.
    // Stop la budget < min_director_cost sau safety limit.
    std::vector<EnemySpec> generateSpawns(int budget) const;

    bool empty() const { return pool_.empty(); }
    const std::vector<std::string>& unlockedKeys() const { return unlocked_keys_; }
};
