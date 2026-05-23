#pragma once
#include <string>
#include <unordered_map>
#include "TowerSpec.h"
#include "EnemySpec.h"
#include "WaveSpec.h"
#include "MapSpec.h"

// Container pentru toate specs incarcate din data/*.json la startup.
// Game-ul detine o instanta DataRegistry si pass-uieste const ref la spec
// fiecarui Tower / Enemy creat.
//
// Aruncata DataException daca:
//   - JSON lipseste
//   - parse error
//   - cheia ceruta nu exista in registry
class DataRegistry {
    std::unordered_map<std::string, TowerSpec> towers_;
    std::unordered_map<std::string, EnemySpec> enemies_;
    std::unordered_map<std::string, WaveSpec>  waves_;
    std::unordered_map<std::string, MapSpec>   maps_;

public:
    DataRegistry() = default;

    // Incarca toate JSON-urile din default "data"
    // Apelata o data, la startup.
    void loadAll(const std::string& dataDir = "data");

    // Lookup. Arunca DataException daca cheia lipseste.
    const TowerSpec& getTower(const std::string& key) const;
    const EnemySpec& getEnemy(const std::string& key) const;
    const WaveSpec&  getWave(const std::string& key)  const;
    const MapSpec&   getMap(const std::string& key)   const;

    // Verificare existenta fara throw (util pt UI/Director).
    bool hasTower(const std::string& key) const;
    bool hasEnemy(const std::string& key) const;
    bool hasWave(const std::string& key)  const;
    bool hasMap(const std::string& key)   const;
};
