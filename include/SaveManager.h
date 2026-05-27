#pragma once
#include <string>
#include "SaveData.h"

// I/O simplu pe data/save_current.json. Format JSON serializat prin macro-urile
// NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE din SaveData.h.
//
// Folosit de GameScene (autosave la wave end / shop purchase / tower place,
// load la Continue) si de MainMenuScene (verificare existenta pentru activare buton).
namespace SaveManager {
    constexpr const char* SAVE_PATH = "data/save_current.json";
    bool saveExists();
    void write(const SaveData& data);
    SaveData read();
    void deleteSave();
}
