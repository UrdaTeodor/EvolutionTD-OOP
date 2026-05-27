#include "DataRegistry.h"
#include "GameException.h"

#include <fstream>
#include <nlohmann/json.hpp>

namespace {
    // Helper template incarca un JSON object dintr-un fisier si umple un map<string, T>.
    // Instantieri reale: TowerSpec, EnemySpec, WaveSpec, MapSpec
    // Foloseste from_json definit in <T>Spec.h cu ADL
    template <typename T>
    void loadIntoMap(const std::string& path, std::unordered_map<std::string, T>& out) {
        std::ifstream stream(path);
        if (!stream.is_open()) {
            throw DataException("Nu pot deschide fisierul JSON: " + path);
        }
        nlohmann::json j;
        try {
            stream >> j;
        } catch (const nlohmann::json::parse_error& e) {
            throw DataException("JSON malformat in " + path + ": " + e.what());
        }
        for (auto it = j.begin(); it != j.end(); ++it) {
            out.emplace(it.key(), it.value().get<T>());
        }
    }
}

void DataRegistry::loadAll(const std::string& dataDir) {
    loadIntoMap<TowerSpec>(dataDir + "/towers.json",  towers_);
    loadIntoMap<EnemySpec>(dataDir + "/enemies.json", enemies_);
    loadIntoMap<WaveSpec> (dataDir + "/waves.json",   waves_);
    loadIntoMap<MapSpec>  (dataDir + "/maps.json",    maps_);
}

const TowerSpec& DataRegistry::getTower(const std::string& key) const {
    auto it = towers_.find(key);
    if (it == towers_.end()) {
        throw DataException("Tower spec lipseste din registry: '" + key + "'");
    }
    return it->second;
}

const EnemySpec& DataRegistry::getEnemy(const std::string& key) const {
    auto it = enemies_.find(key);
    if (it == enemies_.end()) {
        throw DataException("Enemy spec lipseste din registry: '" + key + "'");
    }
    return it->second;
}

const WaveSpec& DataRegistry::getWave(const std::string& key) const {
    auto it = waves_.find(key);
    if (it == waves_.end()) {
        throw DataException("Wave spec lipseste din registry: '" + key + "'");
    }
    return it->second;
}

const MapSpec& DataRegistry::getMap(const std::string& key) const {
    auto it = maps_.find(key);
    if (it == maps_.end()) {
        throw DataException("Map spec lipseste din registry: '" + key + "'");
    }
    return it->second;
}

bool DataRegistry::hasEnemy(const std::string& key) const {
    return enemies_.find(key) != enemies_.end();
}

std::vector<std::string> DataRegistry::towerKeys() const {
    std::vector<std::string> keys;
    keys.reserve(towers_.size());
    for (const auto& [key, _] : towers_) keys.push_back(key);
    return keys;
}
