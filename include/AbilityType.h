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
        AMPLIFY,              // Honeypot suport: +15% damage turnurilor din aura

        // ---- Legendary (3 combinatii valide pentru Mythic) ----
        DOUBLE_SHOT,          // atac: rafala de 2 + proiectilele STRAPUNG tinta (pierce clasic)
        FIRE_TRAIL,           // atac: arsura pe tinta (0.9 dmg/s x3s pe lovitura directa)
        KNOCKBACK_EVERY_3,    // atac: la fiecare al 3-lea shot, knockback
        REFLECTIVE_SHIELD,    // redirectioneaza inamicul inapoi pe track si scade hp ramas din hp urmatorului inamic (cooldown 5s)(merge maxim 3 patratele inapoi)
        MOVABLE,              // orice turn: poate fi mutat o data per val
        OVERCLOCK,            // Honeypot suport: +25% attack speed turnurilor din aura
        SPLASH                // Antivirus: impactul face damage AoE in jurul tintei
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
        case AbilityType::AMPLIFY:           return "AMPLIFY";
        case AbilityType::OVERCLOCK:         return "OVERCLOCK";
        case AbilityType::SPLASH:            return "SPLASH";
    }
    return "?";
}

// Nume prietenos pentru UI (info panel, tooltips) — enum-ul ramane pentru JSON.
inline const char* abilityDisplayName(AbilityType a) {
    switch (a) {
        case AbilityType::MULTI_TARGET:      return "MultiTarget";
        case AbilityType::BIGGER_AURA:       return "BiggerAura";
        case AbilityType::ARMORED:           return "Armored";
        case AbilityType::AMPLIFY:           return "Amplify";
        case AbilityType::DOUBLE_SHOT:       return "DoubleShot";
        case AbilityType::FIRE_TRAIL:        return "FireTrail";
        case AbilityType::KNOCKBACK_EVERY_3: return "Knockback";
        case AbilityType::REFLECTIVE_SHIELD: return "ReflectShield";
        case AbilityType::MOVABLE:           return "Movable";
        case AbilityType::OVERCLOCK:         return "Overclock";
        case AbilityType::SPLASH:            return "BlastWave";
    }
    return "?";
}

// Ce face efectiv evolutia, cu numerele reale — afisat la hover (info panel,
// token popup). Daca jocul te lasa sa aplici ceva, trebuie sa-ti si spuna ce face.
inline const char* abilityDescription(AbilityType a) {
    switch (a) {
        case AbilityType::MULTI_TARGET:
            return "Attacks +1 enemy. Stacks";
        case AbilityType::BIGGER_AURA:
            return "Honeypot: +50% aura radius, slow 20% -> 30%. Does not stack.";
        case AbilityType::ARMORED:
            return "Firewall: blocks cost only 50% of the firewall's HP. Does not stack.";
        case AbilityType::AMPLIFY:
            return "Honeypot: +15% damage to towers inside the aura. Stacks on the same honeypot.";
        case AbilityType::DOUBLE_SHOT:
            return "+1 shot (60% dmg); shots pierce to the enemy behind the target (50% dmg). Stacks.";
        case AbilityType::FIRE_TRAIL:
            return "Ignites the target on hit: 90% of damage per second for 3s. Stacks.";
        case AbilityType::KNOCKBACK_EVERY_3:
            return "Every 3rd hit knocks the enemy back (max 2/s; bosses immune). Does not stack.";
        case AbilityType::REFLECTIVE_SHIELD:
            return "Firewall: shoves the enemy 3 cells back, 5s cooldown. Does not stack.";
        case AbilityType::MOVABLE:
            return "Tower can be moved with M, even mid-wave. Does not stack.";
        case AbilityType::OVERCLOCK:
            return "Honeypot: +25% attack speed to towers inside the aura. Stacks on the same honeypot.";
        case AbilityType::SPLASH:
            return "Impact deals 35% of damage to all enemies within 1.2 cells of the target. Stacks.";
    }
    return "";
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
    if (s == "AMPLIFY")           return AbilityType::AMPLIFY;
    if (s == "OVERCLOCK")         return AbilityType::OVERCLOCK;
    if (s == "SPLASH")            return AbilityType::SPLASH;
    throw std::invalid_argument("AbilityType necunoscut: " + s);
}
