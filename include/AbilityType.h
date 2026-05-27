#pragma once
#include <string>
#include <stdexcept>

// Enum global pentru ability types. Tower base trebuie sa stie
// AbilityType pentru virtual applyAbility(AbilityType), dar nu ar trebui sa includa
// Evolution.h.
//
// AbilityEvolution.h pastreaza un alias `AbilityEvolution::AbilityType` pentru
// compatibilitate cu codul existent.
enum class AbilityType {
        // ---- Epic ----
        MULTI_TARGET,         // dubleaza numarul de atacuri simultane
                              //(ex. 2 proiectile in loc de 1 dar pe inamici diferiti,
                              // nu schimba cu nimic daca exista un singur inamic in raza)
        BIGGER_AURA,          // Honeypot: raza permanent mai mare
        ARMORED,              // flat damage reduction (da reward sa tankezi inamici multi si slabi)

        // ---- Legendary (3 combinatii valide pentru Mythic) ----
        DOUBLE_SHOT,          // atac: 2 proiectile la 80% dmg fiecare
        FIRE_TRAIL,           // atac: proiectila lasa urma de foc (20% dmg pe secunda timp de 3 sec)
        KNOCKBACK_EVERY_3,    // atac: la fiecare al 3-lea shot, knockback
        REFLECTIVE_SHIELD,    // redirectioneaza inamicul inapoi pe track si scade hp ramas din hp urmatorului inamic (cooldown 5s)(merge maxim 3 patratele inapoi)
        MOVABLE               // orice turn: poate fi mutat o data per val
};

// Conversie ability <-> string. Folosit la incarcare JSON (data/evolutions.json,
// data/towers.json supports_abilities) si la afisare UI carduri shop.
inline const char* abilityToString(AbilityType a) {
    switch (a) {
        case AbilityType::MULTI_TARGET:      return "MULTI_TARGET";
        case AbilityType::BIGGER_AURA:       return "BIGGER_AURA";
        case AbilityType::ARMORED:           return "ARMORED";
        case AbilityType::DOUBLE_SHOT:       return "DOUBLE_SHOT";
        case AbilityType::FIRE_TRAIL:        return "FIRE_TRAIL";
        case AbilityType::KNOCKBACK_EVERY_3: return "KNOCKBACK_EVERY_3";
        case AbilityType::REFLECTIVE_SHIELD: return "REFLECTIVE_SHIELD";
        case AbilityType::MOVABLE:           return "MOVABLE";
    }
    return "?";
}

// Arunca std::invalid_argument daca string-ul nu corespunde nici unei valori.
// Apelata din from_json pentru MajorEvolutionSpec si TowerSpec.supports_abilities.
inline AbilityType stringToAbility(const std::string& s) {
    if (s == "MULTI_TARGET")      return AbilityType::MULTI_TARGET;
    if (s == "BIGGER_AURA")       return AbilityType::BIGGER_AURA;
    if (s == "ARMORED")           return AbilityType::ARMORED;
    if (s == "DOUBLE_SHOT")       return AbilityType::DOUBLE_SHOT;
    if (s == "FIRE_TRAIL")        return AbilityType::FIRE_TRAIL;
    if (s == "KNOCKBACK_EVERY_3") return AbilityType::KNOCKBACK_EVERY_3;
    if (s == "REFLECTIVE_SHIELD") return AbilityType::REFLECTIVE_SHIELD;
    if (s == "MOVABLE")           return AbilityType::MOVABLE;
    throw std::invalid_argument("AbilityType necunoscut: " + s);
}
