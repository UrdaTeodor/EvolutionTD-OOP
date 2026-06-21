#include "Sprites.h"
#include "Enemy.h"
#include <algorithm>
#include <cmath>

void Sprites::load() {
    const char* towerFiles[5] = {
        "assets/sprites/tower_antivirus.png",
        "assets/sprites/tower_adblocker.png",
        "assets/sprites/tower_honeypot.png",
        "assets/sprites/tower_firewall.png",
        "assets/sprites/tower_miner.png",
    };
    const char* enemyFiles[4] = {
        "assets/sprites/enemy_adware.png",
        "assets/sprites/enemy_trojan.png",
        "assets/sprites/enemy_worm.png",
        "assets/sprites/enemy_iloveyou.png",
    };
    const char* bossFiles[5] = {
        "assets/sprites/enemy_iloveyou_p1.png",
        "assets/sprites/enemy_iloveyou_p2.png",
        "assets/sprites/enemy_iloveyou_p3.png",
        "assets/sprites/enemy_iloveyou_p4.png",
        "assets/sprites/enemy_iloveyou_p5.png",
    };
    for (int i = 0; i < 5; i++) towerOk[i]     = towerTex[i].loadFromFile(towerFiles[i]);
    for (int i = 0; i < 4; i++) enemyOk[i]     = enemyTex[i].loadFromFile(enemyFiles[i]);
    for (int i = 0; i < 5; i++) bossPhaseOk[i] = bossPhases[i].loadFromFile(bossFiles[i]);
    fireTrailOk = fireTrailTex.loadFromFile("assets/sprites/fire_trail.png");

    // Glow radial: alpha cade patratic de la centru spre margine (nucleu
    // luminos, halo moale). smooth ca scalarea mare sa nu arate pixelat.
    {
        constexpr unsigned N = 64;
        sf::Image img;
        img.create(N, N, sf::Color::Transparent);
        const float c = (N - 1) / 2.0f;
        for (unsigned yy = 0; yy < N; ++yy) {
            for (unsigned xx = 0; xx < N; ++xx) {
                float dx = (static_cast<float>(xx) - c) / c;
                float dy = (static_cast<float>(yy) - c) / c;
                float falloff = std::max(0.0f, 1.0f - std::sqrt(dx * dx + dy * dy));
                falloff *= falloff;
                img.setPixel(xx, yy, sf::Color(255, 255, 255,
                             static_cast<sf::Uint8>(255.0f * falloff)));
            }
        }
        glowTex.loadFromImage(img);
        glowTex.setSmooth(true);
    }
}

int Sprites::towerIndex(char c) {
    switch (c) {
        case 'A': return 0;
        case 'D': return 1;
        case 'H': return 2;
        case 'F': return 3;
        case 'M': return 4;
        default:  return -1;
    }
}

int Sprites::enemyIndex(const std::string& name) {
    if (name == "Adware")   return 0;
    if (name == "Trojan")   return 1;
    if (name == "Worm")     return 2;
    if (name == "ILOVEYOU") return 3;
    return -1;
}

float Sprites::displayScale(const std::string& name) {
    if (name == "ILOVEYOU") return 2.0f;
    if (name == "Trojan")   return 1.4f;
    if (name == "Worm")     return 0.8f;
    return 1.0f;
}

int Sprites::bossPhaseForRatio(float ratio) {
    if (ratio > 0.8f) return 0;
    if (ratio > 0.6f) return 1;
    if (ratio > 0.4f) return 2;
    if (ratio > 0.2f) return 3;
    return 4;
}

const sf::Texture* Sprites::textureFor(const Enemy& enemy) const {
    const std::string& name = enemy.getName();
    if (name == "ILOVEYOU") {
        float ratio = (enemy.getMaxHealth() > 0.0f)
                          ? (enemy.getCurrentHealth() / enemy.getMaxHealth())
                          : 0.0f;
        int phase = bossPhaseForRatio(ratio);
        if (bossPhaseOk[phase]) return &bossPhases[phase];
        if (enemyOk[3])         return &enemyTex[3];
        return nullptr; // 5 health stages
    }
    int idx = enemyIndex(name);
    if (idx >= 0 && enemyOk[idx]) return &enemyTex[idx];
    return nullptr;
}
