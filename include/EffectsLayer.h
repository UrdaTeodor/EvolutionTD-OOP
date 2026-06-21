#pragma once
#include <SFML/Graphics.hpp>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>
#include "VisualEvent.h"

class Game;
struct Sprites;
class AudioManager;

// Efecte vizuale. Dupa refactorul v0.4.2, proiectilele NU mai sunt simulate
// aici: sunt entitati de logica (Shot, in Wave), iar EffectsLayer le deseneaza.
// v0.4.4 duce principiul "vizual = adevar" mai departe: Wave emite VisualEvent
// pentru fiecare tragere / impact / moarte, iar tot ce explodeaza pe ecran
// vine din evenimentele alea — nu din diff-uri de HP ghicite. Damage numbers
// raman pe diff de HP (acopera si DoT-urile, fractionare per frame).
// Tot ce e "lumina" se deseneaza aditiv (sf::BlendAdd) cu Sprites::glowTex.
class EffectsLayer {
public:
    EffectsLayer(const Sprites& sprites, const sf::Font* font, AudioManager* audio);

    // events = ce s-a intamplat REAL in simulare in acest frame (din Wave).
    void update(const Game& game, bool waveRunning, float dt,
                std::vector<VisualEvent> events);

    // Monedele de income (endWave): pop din fiecare Miner, apoi zboara spre
    // contorul de bani (counter_px). Monede desenate = bani realmente primiti.
    void spawnIncomeCoins(const std::vector<IncomeEvent>& events,
                          sf::Vector2f counter_px);

    void renderTrails(sf::RenderWindow& window) const;
    void renderForeground(sf::RenderWindow& window, const Game& game) const;

    // Recoil 0..1 pe turnul dat (1 = tocmai a tras); BoardRenderer umfla
    // scurt sprite-ul cu el (squash & stretch).
    float towerRecoil(int tower_id) const;

    // Timp animat global, partajat cu BoardRenderer (pulsatiile aurelor).
    float time() const { return elapsed_; }

    // Screen shake cerut de efectele mari (firestorm, boss kill);
    // GameScene il consuma o data pe frame.
    float takeShakeRequest();

    // Reset transients (undo / final de val).
    void clear();

private:
    // Particula generica desenata aditiv cu glowTex (scanteie, foc, moarte).
    struct Particle {
        sf::Vector2f pos;
        sf::Vector2f vel;
        float age      = 0.0f;
        float lifetime = 0.4f;
        float size     = 10.0f;   // diametru px la age=0, scade liniar spre 0
        float drag     = 0.0f;    // franare exponentiala
        float gravity  = 0.0f;    // px/s^2 (negativ = pluteste in sus)
        sf::Color color;
    };

    // Inel care se extinde: shockwave Bruiser, firestorm, moarte de boss.
    struct Ring {
        sf::Vector2f pos;
        float age = 0.0f, lifetime = 0.4f;
        float r0 = 4.0f, r1 = 60.0f;
        float thickness = 3.0f;
        sf::Color color;
    };

    // "+$X" auriu la kill; damage numbers colorate pe stare.
    struct FloatingText {
        sf::Vector2f pos;
        float age = 0.0f, lifetime = 0.8f;
        std::string text;
        unsigned size = 17;
        sf::Color color;
    };

    // Muzzle flash care asteapta delay-ul rafalei DoubleShot.
    struct PendingMuzzle {
        float delay = 0.0f;
        VisualEvent ev;
    };

    // Moneda de income: faza 1 = tasneste cu gravitatie, faza 2 = zboara
    // accelerat spre contor; la sosire, ding cu pitch crescator.
    struct Coin {
        sf::Vector2f pos, vel;
        float age      = 0.0f;
        float pop_time = 0.45f;
        bool  flying   = false;
        int   pitch_step = 0;
    };

    struct TrailParticle {
        sf::Vector2f pos;
        float        age;
        float        lifetime;
    };

    const Sprites&  sprites_;
    const sf::Font* font_;   // nullptr => texte dezactivate
    AudioManager*   audio_;  // nullptr => fara sunete

    float        elapsed_       = 0.0f;
    float        shake_request_ = 0.0f;
    float        screen_flash_  = 0.0f;   // firestorm/boss: flash aditiv pe tot ecranul
    sf::Color    flash_color_   = sf::Color(255, 140, 40);
    sf::Vector2f coin_target_{90.0f, 128.0f};

    // HP-ul cunoscut per enemy id + damage fractionar neemis (DoT < 1/frame).
    std::unordered_map<int, float> known_hp_;
    std::unordered_map<int, float> pending_dmg_;

    std::unordered_map<int, float> recoil_;        // tower id -> timp ramas
    std::unordered_map<int, int>   prev_embers_;   // tower id -> embers frame trecut

    std::vector<Particle>      particles_;
    std::vector<Ring>          rings_;
    std::vector<FloatingText>  texts_;
    std::vector<PendingMuzzle> pending_muzzles_;
    std::vector<Coin>          coins_;
    std::vector<TrailParticle> trail_particles_;

    float trail_accum_ = 0.0f;
    float burn_accum_  = 0.0f;

    // RNG pur vizual (separat de RNG-ul de gameplay seedabil din Game).
    mutable std::mt19937 rng_{std::random_device{}()};
    float frand(float a, float b);

    void handleEvent(const VisualEvent& ev);
    void fireMuzzle(const VisualEvent& ev);
    void impactBurst(const VisualEvent& ev);
    void deathBurst(const VisualEvent& ev);

    // Orbitele de embers + detectia firestorm-ului (embers au scazut = a tras).
    void updatePhoenix(const Game& game);

    void detectDamage(const Game& game);
};
