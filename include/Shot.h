#pragma once

// Stilurile vizuale ale unui proiectil, setate la tragere din evolutiile
// turnului. Regula de tier: un Mythic ASCUNDE legendarele-ingredient
// (Phoenix ascunde DoubleShot+FireTrail, Bruiser ascunde Knockback),
// dar celelalte evolutii raman vizibile.
namespace shotstyle {
    constexpr int PHOENIX     = 1 << 0;   // mythic PhoenixBarrage
    constexpr int BRUISER     = 1 << 1;   // mythic RovingBruiser
    constexpr int DOUBLE_SHOT = 1 << 2;   // legendar
    constexpr int FIRE_TRAIL  = 1 << 3;   // legendar
    constexpr int KNOCKBACK   = 1 << 4;   // legendar
    constexpr int EPIC        = 1 << 5;   // abilitati epice (MultiTarget)
    constexpr int SPLASH      = 1 << 6;   // legendar BlastWave (AoE la impact)
}

// Proiectil de LOGICA (nu vizual): damage-ul si efectele se aplica la IMPACT.
// Homing: urmareste inamicul dupa id; daca tinta moare in zbor, proiectilul
// dispare fara efect (fizzle) — overkill-ul devine risipa reala.
// Traieste in Wave; EffectsLayer doar il deseneaza (vizual = adevar).
struct Shot {
    int   target_id   = -1;      // Enemy::id urmarit
    float x = 0.0f, y = 0.0f;    // pozitie curenta (coordonate celule: col, row)
    float speed       = 8.0f;    // celule / secunda
    float damage      = 0.0f;

    // Payload aplicat la impact:
    float fire_dps    = 0.0f;    // >0 => applyFireTrail(fire_dps, 3s)
    bool  knockback   = false;   // pushBack(knock_cells)
    int   knock_cells = 1;
    bool  shockwave   = false;   // RovingBruiser: AoE push + vulnerabilitate
    bool  excess_slow = false;   // anti-cheese: slow 50%/1s in loc de knockback
    float splash_pct  = 0.0f;    // BlastWave: % din damage aplicat AoE langa tinta
    // Pierce clasic (DoubleShot): cate strapungeri mai are proiectilul. La
    // impact, daca >0, isi cauta o tinta NOUA in spatele celei lovite (pe
    // directia de zbor) si continua cu damage injumatatit.
    int   pierce_left = 0;

    float delay       = 0.0f;    // rafala DoubleShot: porneste dupa delay sec

    // Pentru randare + atribuirea ember-urilor Phoenix:
    char  source_char     = 'A';
    int   source_tower_id = -1;
    int   style_mask      = 0;   // biti din shotstyle
};
