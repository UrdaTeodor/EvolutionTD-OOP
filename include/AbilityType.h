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
            return "Ataca +1 inamic simultan, la 100% damage. Se stackeaza (+1 tinta per token).";
        case AbilityType::BIGGER_AURA:
            return "Honeypot: aura cu 50% mai mare si slow 30% (de la 20%). NU se stackeaza.";
        case AbilityType::ARMORED:
            return "Firewall: blocarile costa doar 50% din HP-ul firewall-ului. NU se stackeaza.";
        case AbilityType::AMPLIFY:
            return "Honeypot: +15% damage pentru turnurile din aura. Se stackeaza pe acelasi honeypot.";
        case AbilityType::DOUBLE_SHOT:
            return "Rafala: +1 foc (60% dmg) si proiectilele strapung inamicul din spatele tintei (50% dmg). Se stackeaza.";
        case AbilityType::FIRE_TRAIL:
            return "Aprinde tinta la impact: 90% din damage pe secunda, timp de 3s. Se stackeaza.";
        case AbilityType::KNOCKBACK_EVERY_3:
            return "La fiecare a 3-a lovitura inamicul e impins inapoi (max 2/s; bosii imuni). NU se stackeaza.";
        case AbilityType::REFLECTIVE_SHIELD:
            return "Firewall: arunca inamicul 3 celule inapoi, cooldown 5s. NU se stackeaza.";
        case AbilityType::MOVABLE:
            return "Turnul poate fi mutat cu tasta M, inclusiv in timpul valului. NU se stackeaza.";
        case AbilityType::OVERCLOCK:
            return "Honeypot: +25% viteza de atac pentru turnurile din aura. Se stackeaza pe acelasi honeypot.";
        case AbilityType::SPLASH:
            return "Impactul face 35% din damage tuturor inamicilor pe 1.2 celule in jurul tintei. Se stackeaza.";
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
