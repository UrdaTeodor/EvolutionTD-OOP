#pragma once
#include <SFML/Graphics/Texture.hpp>
#include <string>

class Enemy;

// Depozit de texturi incarcate o singura data la pornirea GameScene.
// Mutat din GameScene (era struct intern) ca sa poata fi folosit si de
// BoardRenderer / EffectsLayer fara sa depinda de GameScene.
// Fallback: daca o textura lipseste, flag-ul *Ok ramane false si
// caller-ul deseneaza forme geometrice colorate in loc.
struct Sprites {
    sf::Texture towerTex[5];
    bool        towerOk[5] = {false, false, false, false, false};
    sf::Texture enemyTex[4];
    bool        enemyOk[4] = {false, false, false, false};
    sf::Texture bossPhases[5];
    bool        bossPhaseOk[5] = {false, false, false, false, false};
    sf::Texture fireTrailTex;
    bool        fireTrailOk = false;

    // Glow radial alb, generat procedural in load() (nu exista pe disc).
    // Desenat cu sf::BlendAdd si tintat cu setColor => orice particula/aura
    // luminoasa din joc; fara el, "lumina" facuta din CircleShape are margini
    // dure si nu se aduna corect.
    sf::Texture glowTex;

    void load();

    static int towerIndex(char c);
    static int enemyIndex(const std::string& name);
    static float displayScale(const std::string& name);
    static int bossPhaseForRatio(float ratio);

    const sf::Texture* textureFor(const Enemy& enemy) const;
};
