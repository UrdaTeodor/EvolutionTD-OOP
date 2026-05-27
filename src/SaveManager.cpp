#include "SaveManager.h"
#include "GameException.h"
#include <cstdio>
#include <fstream>
#include <nlohmann/json.hpp>

namespace SaveManager {

bool saveExists() {
    std::ifstream in(SAVE_PATH);
    return in.good();
}

void write(const SaveData& data) {
    std::ofstream out(SAVE_PATH);
    if (!out) {
        throw DataException("SaveManager.write: nu pot scrie in " + std::string(SAVE_PATH));
    }
    nlohmann::json j = data;
    out << j.dump(2);   // pretty-print cu indent 2
}

SaveData read() {
    std::ifstream in(SAVE_PATH);
    if (!in) {
        throw DataException("SaveManager.read: nu pot deschide " + std::string(SAVE_PATH));
    }
    nlohmann::json j;
    try {
        in >> j;
    } catch (const nlohmann::json::parse_error& err) {
        throw DataException("SaveManager.read: JSON malformat: " + std::string(err.what()));
    }
    try {
        return j.get<SaveData>();
    } catch (const std::exception& err) {
        throw DataException("SaveManager.read: save incompatibil sau corupt: " + std::string(err.what()));
    }
}

void deleteSave() {
    std::remove(SAVE_PATH);   // no-op daca lipseste; nu verificam result
}

} // namespace SaveManager
