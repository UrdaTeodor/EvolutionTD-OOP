#pragma once
#include <string>

// Tipurile de evolutii Mythic (craft din 2 Legendary, retetele in evolutions.json).
// Enum separat de MythicEvolution ca sa-l poata folosi si Tower (applyMythic)
// fara sa traga toata ierarhia de Evolution in header.
enum class MythicType { PHOENIX_BARRAGE, ROVING_BRUISER, SHIELDED_RUNNER };

inline const char* mythicTypeName(MythicType m) {
    switch (m) {
        case MythicType::PHOENIX_BARRAGE: return "PhoenixBarrage";
        case MythicType::ROVING_BRUISER:  return "RovingBruiser";
        case MythicType::SHIELDED_RUNNER: return "ShieldedRunner";
    }
    return "?";
}

inline bool mythicTypeFromString(const std::string& s, MythicType& out) {
    if (s == "PhoenixBarrage") { out = MythicType::PHOENIX_BARRAGE; return true; }
    if (s == "RovingBruiser")  { out = MythicType::ROVING_BRUISER;  return true; }
    if (s == "ShieldedRunner") { out = MythicType::SHIELDED_RUNNER; return true; }
    return false;
}
