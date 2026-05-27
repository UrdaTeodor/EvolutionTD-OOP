#pragma once
#include <random>
#include <vector>
#include <string>
#include "EnemySpec.h"
#include "WeightedTable.hpp"

// Directorul tine un pool de inamici deblocati progressiv prin
// unlocks din WaveSpec. La build wave, primeste un budget si cheltuie pe spawnuri random ponderate.
// Game ii dA referinta la RNG-ul propriu (seedable).
class Director {
    WeightedTable<EnemySpec> pool_;
    std::mt19937*            rng_;     //(seed)
    std::vector<std::string> unlocked_keys_;  

public:
    explicit Director(std::mt19937& rng);

    //dupa copy/swap pe Game, rng_ pointerul ramane si trb corectat
    // cppcheck-suppress unusedFunction
    void setRng(std::mt19937& rng){rng_ = &rng;}

    // Adauga un EnemySpec in pool (apelat la endWave dupa ce wave_N.unlocks_after se aplica).
    // weight default 1.0. 
    void unlock(const std::string& key, const EnemySpec& spec, float weight = 1.0f);

    // Genereaza o lista de EnemySpec spawnari pentru un wave,duoa buget.
    std::vector<EnemySpec> generateSpawns(int budget) const;

    bool empty() const { return pool_.empty(); }
};
