#pragma once
#include <string>

// Evenimente vizuale emise de LOGICA (Wave/Game) si consumate de EffectsLayer.
// Extinde principiul "vizual = adevar" de la Shot: pana acum EffectsLayer ghicea
// loviturile din diff-uri de HP si nu stia CAUZA (orice hit arata identic).
// Acum simularea raporteaza ce s-a intamplat real, cu tot contextul (stil,
// damage, kill), iar efectele se deseneaza din asta. Header fara SFML: e parte
// din game core (folosit si de balance_sim, care nu linkeaza grafica).
struct VisualEvent {
    enum class Type { SHOT_FIRED, IMPACT, DEATH };

    Type  type = Type::IMPACT;
    float x = 0.0f, y = 0.0f;      // coordonate celula (col, row)

    // SHOT_FIRED + IMPACT:
    int   style_mask = 0;          // biti din shotstyle (stilul proiectilului)

    // SHOT_FIRED (recoil + muzzle flash + sunet pe turnul-sursa):
    int   source_tower_id = -1;
    char  source_char     = 'A';
    float delay           = 0.0f;  // rafala DoubleShot: flash-ul porneste decalat

    // IMPACT:
    float damage    = 0.0f;        // scaleaza marimea exploziei
    bool  killed    = false;       // impactul a ucis tinta
    bool  knockback = false;       // dare directionale pe directia push-ului
    bool  shockwave = false;       // unda Bruiser: inel real pe raza de efect
    float dir_x = 0.0f, dir_y = 0.0f;   // directia proiectilului la impact

    // DEATH (orice cauza, inclusiv DoT — emis cand inamicul moare in simulare):
    std::string enemy_name;        // culoarea exploziei de moarte
    int reward = 0;                // "+$X" auriu peste locul mortii
};

// Venit pasiv incasat la endWave, per turn (Miner): monedele care tasnesc din
// turn si zboara spre contorul de bani din HUD. Monede desenate = bani reali.
struct IncomeEvent {
    float x = 0.0f, y = 0.0f;      // celula turnului
    int   amount = 0;
};
