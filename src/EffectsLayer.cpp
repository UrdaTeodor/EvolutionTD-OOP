#include "EffectsLayer.h"
#include "AudioManager.h"
#include "Game.h"
#include "Sprites.h"
#include "Enemy.h"
#include "ProjectileTower.h"
#include "Shot.h"
#include "Palette.h"
#include "UiLayout.h"
#include <algorithm>
#include <cmath>

namespace {

    constexpr float RECOIL_TIME = 0.12f;

    // Capuri anti-supa la 4x speed in endless (sute de evenimente/secunda).
    constexpr size_t MAX_PARTICLES = 800;
    constexpr size_t MAX_TEXTS     = 150;

    sf::Color baseShotColor(const Shot& s) {
        sf::Color c = (s.source_char == 'A') ? sf::Color(150, 220, 255)
                                             : sf::Color(255, 240, 100);
        if (s.style_mask & shotstyle::FIRE_TRAIL) c = sf::Color(255, 150, 80);
        if (s.style_mask & shotstyle::PHOENIX)    c = sf::Color(255, 250, 235);
        return c;
    }

    // Culoarea exploziei de impact, dupa stilul proiectilului.
    sf::Color impactColor(int style) {
        if (style & shotstyle::PHOENIX)     return sf::Color(255, 235, 190);
        if (style & shotstyle::FIRE_TRAIL)  return sf::Color(255, 150, 60);
        if (style & shotstyle::BRUISER)     return sf::Color(255, 110, 90);
        if (style & shotstyle::KNOCKBACK)   return sf::Color(255, 170, 70);
        if (style & shotstyle::SPLASH)      return sf::Color(255, 190, 110);
        if (style & shotstyle::DOUBLE_SHOT) return sf::Color(255, 215, 90);
        if (style & shotstyle::EPIC)        return sf::Color(195, 115, 255);
        return sf::Color(170, 220, 255);
    }

    // Glow aditiv: alpha moduleaza intensitatea (BlendAdd = src*alpha + dst).
    void drawGlow(sf::RenderWindow& window, const sf::Texture& tex,
                  sf::Vector2f pos, float size_px, sf::Color color) {
        sf::Sprite spr(tex);
        sf::Vector2u ts = tex.getSize();
        spr.setOrigin(ts.x / 2.0f, ts.y / 2.0f);
        spr.setScale(size_px / ts.x, size_px / ts.y);
        spr.setPosition(pos);
        spr.setColor(color);
        window.draw(spr, sf::RenderStates(sf::BlendAdd));
    }

    void drawRing(sf::RenderWindow& window, sf::Vector2f center, float radius,
                  float thickness, sf::Color col, bool additive = true) {
        sf::CircleShape ring(radius);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(thickness);
        ring.setOutlineColor(col);
        ring.setPosition(center.x - radius, center.y - radius);
        if (additive) window.draw(ring, sf::RenderStates(sf::BlendAdd));
        else          window.draw(ring);
    }

} // namespace

EffectsLayer::EffectsLayer(const Sprites& sprites, const sf::Font* font,
                           AudioManager* audio)
    : sprites_(sprites), font_(font), audio_(audio) {}

float EffectsLayer::frand(float a, float b) {
    std::uniform_real_distribution<float> d(a, b);
    return d(rng_);
}

void EffectsLayer::clear() {
    trail_particles_.clear();
    particles_.clear();
    rings_.clear();
    texts_.clear();
    pending_muzzles_.clear();
    coins_.clear();
    known_hp_.clear();
    pending_dmg_.clear();
    recoil_.clear();
    prev_embers_.clear();
    screen_flash_ = 0.0f;
}

float EffectsLayer::towerRecoil(int tower_id) const {
    auto it = recoil_.find(tower_id);
    if (it == recoil_.end()) return 0.0f;
    return std::clamp(it->second / RECOIL_TIME, 0.0f, 1.0f);
}

float EffectsLayer::takeShakeRequest() {
    float v = shake_request_;
    shake_request_ = 0.0f;
    return v;
}

// ---------------------------------------------------------------------------
// Evenimente din simulare
// ---------------------------------------------------------------------------

void EffectsLayer::handleEvent(const VisualEvent& ev) {
    switch (ev.type) {
        case VisualEvent::Type::SHOT_FIRED:
            if (ev.delay > 0.0f) pending_muzzles_.push_back({ev.delay, ev});
            else                 fireMuzzle(ev);
            break;
        case VisualEvent::Type::IMPACT:
            impactBurst(ev);
            break;
        case VisualEvent::Type::DEATH:
            deathBurst(ev);
            break;
    }
}

void EffectsLayer::fireMuzzle(const VisualEvent& ev) {
    recoil_[ev.source_tower_id] = RECOIL_TIME;

    sf::Vector2f pos = ui::cellCenter(ev.x, ev.y);
    Particle flash;
    flash.pos      = pos;
    flash.lifetime = 0.10f;
    flash.size     = 30.0f;
    flash.color    = impactColor(ev.style_mask);
    particles_.push_back(flash);

    if (audio_) {
        audio_->play(ev.source_char == 'A' ? Sfx::SHOOT_LASER : Sfx::SHOOT_TICK,
                     30.0f);
    }
}

void EffectsLayer::impactBurst(const VisualEvent& ev) {
    sf::Vector2f pos = ui::cellCenter(ev.x, ev.y);
    sf::Color    col = impactColor(ev.style_mask);

    // Flash central: marimea creste cu damage-ul — un hit de 100 se VEDE
    // diferit de unul de 10, nu doar in cifre.
    Particle flash;
    flash.pos      = pos;
    flash.lifetime = 0.14f;
    flash.size     = 16.0f + std::min(28.0f, ev.damage * 0.35f);
    flash.color    = col;
    particles_.push_back(flash);

    // Scantei radiale, cateva in plus la hituri grele.
    int sparks = 3 + static_cast<int>(std::min(5.0f, ev.damage / 15.0f));
    for (int i = 0; i < sparks; ++i) {
        float ang = frand(0.0f, 6.2831f);
        float spd = frand(60.0f, 170.0f);
        Particle p;
        p.pos      = pos;
        p.vel      = { std::cos(ang) * spd, std::sin(ang) * spd };
        p.lifetime = frand(0.20f, 0.38f);
        p.size     = frand(5.0f, 9.0f);
        p.drag     = 4.0f;
        p.color    = col;
        particles_.push_back(p);
    }

    // Knockback: dare de viteza pe directia loviturii — "pumnul" se vede.
    if (ev.knockback) {
        for (int i = 0; i < 5; ++i) {
            Particle p;
            p.pos      = pos;
            p.vel      = { ev.dir_x * frand(200.0f, 330.0f) + frand(-40.0f, 40.0f),
                           ev.dir_y * frand(200.0f, 330.0f) + frand(-40.0f, 40.0f) };
            p.lifetime = 0.22f;
            p.size     = 7.0f;
            p.drag     = 2.0f;
            p.color    = sf::Color(255, 190, 110);
            particles_.push_back(p);
        }
    }

    // Aprindere FireTrail/Phoenix: embers care se ridica de pe tinta.
    if (ev.style_mask & (shotstyle::FIRE_TRAIL | shotstyle::PHOENIX)) {
        for (int i = 0; i < 4; ++i) {
            Particle p;
            p.pos      = { pos.x + frand(-8.0f, 8.0f), pos.y + frand(-8.0f, 8.0f) };
            p.vel      = { frand(-25.0f, 25.0f), frand(-70.0f, -30.0f) };
            p.lifetime = frand(0.3f, 0.5f);
            p.size     = frand(5.0f, 8.0f);
            p.color    = sf::Color(255, 130, 40);
            particles_.push_back(p);
        }
    }

    // BlastWave: explozie pe raza REALA de splash (1.2 celule) — vezi exact
    // pe cine a lovit AoE-ul.
    if (ev.style_mask & shotstyle::SPLASH) {
        Ring r;
        r.pos       = pos;
        r.lifetime  = 0.3f;
        r.r0        = 6.0f;
        r.r1        = 1.2f * ui::CELL_PX;
        r.thickness = 4.0f;
        r.color     = sf::Color(255, 190, 110);
        rings_.push_back(r);
    }

    // Unda de soc Bruiser: inel REAL pe raza efectiva de 1.5 celule —
    // pana acum push+vulnerabilitatea pe vecini erau invizibile.
    if (ev.shockwave) {
        Ring r;
        r.pos       = pos;
        r.lifetime  = 0.35f;
        r.r0        = 8.0f;
        r.r1        = 1.5f * ui::CELL_PX;
        r.thickness = 5.0f;
        r.color     = sf::Color(255, 120, 80);
        rings_.push_back(r);

        Ring r2 = r;
        r2.lifetime  = 0.5f;
        r2.thickness = 2.5f;
        r2.color     = sf::Color(255, 200, 150);
        rings_.push_back(r2);
    }

    if (audio_) audio_->play(Sfx::HIT, 14.0f + std::min(16.0f, ev.damage * 0.25f));
}

void EffectsLayer::deathBurst(const VisualEvent& ev) {
    sf::Vector2f pos = ui::cellCenter(ev.x, ev.y);
    sf::Color    col = palette::enemyColor(ev.enemy_name);
    bool is_boss     = (ev.enemy_name == "ILOVEYOU");

    // Inamicul nu dispare: explodeaza in bucati de culoarea lui.
    int n = is_boss ? 60 : 12;
    for (int i = 0; i < n; ++i) {
        float ang = frand(0.0f, 6.2831f);
        float spd = frand(50.0f, is_boss ? 360.0f : 190.0f);
        Particle p;
        p.pos      = pos;
        p.vel      = { std::cos(ang) * spd, std::sin(ang) * spd };
        p.lifetime = frand(0.35f, is_boss ? 1.0f : 0.6f);
        p.size     = frand(6.0f, is_boss ? 14.0f : 10.0f);
        p.drag     = 3.0f;
        p.gravity  = 60.0f;
        p.color    = col;
        particles_.push_back(p);
    }

    Particle flash;
    flash.pos      = pos;
    flash.lifetime = 0.16f;
    flash.size     = is_boss ? 130.0f : 36.0f;
    flash.color    = col;
    particles_.push_back(flash);

    if (ev.reward > 0 && font_) {
        FloatingText t;
        t.pos      = { pos.x, pos.y - ui::CELL_PX * 0.2f };
        t.lifetime = 0.9f;
        t.text     = "+$" + std::to_string(ev.reward);
        t.size     = is_boss ? 26u : 15u;
        t.color    = sf::Color(255, 215, 90);
        texts_.push_back(t);
    }

    if (is_boss) {
        Ring r;
        r.pos       = pos;
        r.lifetime  = 0.9f;
        r.r0        = 20.0f;
        r.r1        = 6.0f * ui::CELL_PX;
        r.thickness = 6.0f;
        r.color     = sf::Color(255, 120, 140);
        rings_.push_back(r);
        screen_flash_  = 0.35f;
        flash_color_   = sf::Color(255, 90, 120);
        shake_request_ = std::max(shake_request_, 0.5f);
    }

    if (audio_) audio_->play(Sfx::KILL, is_boss ? 90.0f : 35.0f);
}

// ---------------------------------------------------------------------------
// Phoenix: orbite de embers + firestorm
// ---------------------------------------------------------------------------

void EffectsLayer::updatePhoenix(const Game& game) {
    for (const auto& t : game.getTowers()) {
        const auto* pt = dynamic_cast<const ProjectileTower*>(t.get());
        if (!pt || !pt->mythicBadge() ||
            std::string("PhoenixBarrage") != pt->mythicBadge()) continue;

        int  embers = pt->emberCount();
        auto it     = prev_embers_.find(pt->instanceId());

        // Embers au SCAZUT => firestorm-ul tocmai a pornit (embers_ -= 10).
        if (it != prev_embers_.end() && embers < it->second) {
            auto [vc, vr] = pt->visualPosition();
            sf::Vector2f pos = ui::cellCenter(vc, vr);

            // Val de foc radial care matura toata harta.
            Ring r;
            r.pos       = pos;
            r.lifetime  = 0.8f;
            r.r0        = 10.0f;
            r.r1        = 1.4f * ui::GRID_PX;
            r.thickness = 10.0f;
            r.color     = sf::Color(255, 130, 30);
            rings_.push_back(r);

            Ring r2 = r;
            r2.lifetime = 1.1f;
            r2.thickness = 4.0f;
            r2.color    = sf::Color(255, 220, 120);
            rings_.push_back(r2);

            for (int i = 0; i < 40; ++i) {
                float ang = frand(0.0f, 6.2831f);
                float spd = frand(150.0f, 500.0f);
                Particle p;
                p.pos      = pos;
                p.vel      = { std::cos(ang) * spd, std::sin(ang) * spd };
                p.lifetime = frand(0.5f, 0.9f);
                p.size     = frand(8.0f, 14.0f);
                p.drag     = 2.0f;
                p.color    = sf::Color(255, 140, 40);
                particles_.push_back(p);
            }

            screen_flash_  = 0.45f;
            flash_color_   = sf::Color(255, 130, 30);
            shake_request_ = std::max(shake_request_, 0.4f);
            // Riser-ul de wave-start, pitch-uit in sus = "whoosh" de furtuna.
            if (audio_) audio_->play(Sfx::WAVE_START, 75.0f, 1.4f);
        }
        prev_embers_[pt->instanceId()] = embers;
    }
}

// ---------------------------------------------------------------------------
// Damage numbers (raman pe diff de HP: acopera si DoT-urile)
// ---------------------------------------------------------------------------

void EffectsLayer::detectDamage(const Game& game) {
    const auto& enemies = game.getCurrentWave().getActiveEnemies();

    std::unordered_map<int, float> current;
    current.reserve(enemies.size());

    for (const auto& e : enemies) {
        current[e.id()] = e.getCurrentHealth();

        auto it = known_hp_.find(e.id());
        if (it == known_hp_.end()) continue;   // abia spawnat, nu avem referinta

        float delta = it->second - e.getCurrentHealth();
        if (delta <= 0.0f) continue;

        // Damage-ul fractionar (DoT) se acumuleaza pana ajunge la 1 intreg.
        float& pending = pending_dmg_[e.id()];
        pending += delta;
        if (pending >= 1.0f && font_) {
            int amount = static_cast<int>(pending);
            pending -= static_cast<float>(amount);

            sf::Vector2f c = ui::cellCenter(e.getX(), e.getY());
            FloatingText t;
            t.pos      = { c.x, c.y - ui::CELL_PX * 0.4f };
            t.text     = std::to_string(amount);
            t.size     = (amount >= 100) ? 22u : 17u;
            // Culoarea spune CINE face damage-ul: violet = amplificat de
            // vulnerabilitate, portocaliu = arsura, alb-cald = lovitura normala.
            if      (e.isVulnerable()) t.color = sf::Color(225, 140, 255);
            else if (e.isBurning())    t.color = sf::Color(255, 170, 80);
            else                       t.color = sf::Color(255, 240, 200);
            texts_.push_back(t);
        }
    }

    for (auto it = known_hp_.begin(); it != known_hp_.end();) {
        if (current.find(it->first) == current.end()) {
            pending_dmg_.erase(it->first);
            it = known_hp_.erase(it);
        } else {
            ++it;
        }
    }
    known_hp_ = std::move(current);
}

// ---------------------------------------------------------------------------
// Update / integrare
// ---------------------------------------------------------------------------

void EffectsLayer::update(const Game& game, bool /*waveRunning*/, float dt,
                          std::vector<VisualEvent> events) {
    elapsed_ += dt;

    for (const auto& ev : events) handleEvent(ev);

    // Muzzle flash-urile decalate (rafala DoubleShot).
    for (auto it = pending_muzzles_.begin(); it != pending_muzzles_.end();) {
        it->delay -= dt;
        if (it->delay <= 0.0f) {
            fireMuzzle(it->ev);
            it = pending_muzzles_.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = recoil_.begin(); it != recoil_.end();) {
        it->second -= dt;
        if (it->second <= 0.0f) it = recoil_.erase(it);
        else                    ++it;
    }

    updatePhoenix(game);

    const auto& shots = game.getCurrentWave().getShots();

    // Dara de foc: particule la pozitia proiectilelor cu FireTrail/Phoenix.
    constexpr float TRAIL_SPAWN_INTERVAL = 0.03f;
    constexpr float TRAIL_LIFETIME       = 3.0f;
    trail_accum_ -= dt;
    if (trail_accum_ <= 0.0f) {
        for (const auto& s : shots) {
            if (s.delay > 0.0f) continue;
            if (!(s.style_mask & (shotstyle::FIRE_TRAIL | shotstyle::PHOENIX))) continue;
            trail_particles_.push_back({ ui::cellCenter(s.x, s.y), 0.0f, TRAIL_LIFETIME });
        }
        trail_accum_ = TRAIL_SPAWN_INTERVAL;
    }

    // Flacari pe inamicii care ard: FireTrail-ul se vede pe TINTA, nu doar
    // pe proiectil.
    burn_accum_ -= dt;
    if (burn_accum_ <= 0.0f) {
        for (const auto& e : game.getCurrentWave().getActiveEnemies()) {
            if (!e.isAlive() || !e.isBurning()) continue;
            sf::Vector2f c = ui::cellCenter(e.getX(), e.getY());
            Particle p;
            p.pos      = { c.x + frand(-10.0f, 10.0f), c.y + frand(-8.0f, 8.0f) };
            p.vel      = { frand(-15.0f, 15.0f), frand(-60.0f, -30.0f) };
            p.lifetime = frand(0.25f, 0.45f);
            p.size     = frand(6.0f, 10.0f);
            p.color    = sf::Color(255, 130, 40);
            particles_.push_back(p);
        }
        burn_accum_ = 0.06f;
    }

    detectDamage(game);

    // Integrare particule.
    for (auto& p : particles_) {
        p.age += dt;
        p.vel.y += p.gravity * dt;
        if (p.drag > 0.0f) {
            float k = 1.0f - std::min(1.0f, p.drag * dt);
            p.vel.x *= k;
            p.vel.y *= k;
        }
        p.pos += p.vel * dt;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
        [](const Particle& p) { return p.age >= p.lifetime; }), particles_.end());
    if (particles_.size() > MAX_PARTICLES) {
        particles_.erase(particles_.begin(),
                         particles_.end() - MAX_PARTICLES);
    }

    for (auto& r : rings_) r.age += dt;
    rings_.erase(std::remove_if(rings_.begin(), rings_.end(),
        [](const Ring& r) { return r.age >= r.lifetime; }), rings_.end());

    for (auto& t : texts_) {
        t.age += dt;
        t.pos.y -= 35.0f * dt;
    }
    texts_.erase(std::remove_if(texts_.begin(), texts_.end(),
        [](const FloatingText& t) { return t.age >= t.lifetime; }), texts_.end());
    if (texts_.size() > MAX_TEXTS) {
        texts_.erase(texts_.begin(), texts_.end() - MAX_TEXTS);
    }

    // Monede: pop cu gravitatie, apoi zbor accelerat spre contor.
    for (auto it = coins_.begin(); it != coins_.end();) {
        Coin& c = *it;
        c.age += dt;
        if (!c.flying) {
            c.vel.y += 420.0f * dt;
            c.pos += c.vel * dt;
            if (c.age >= c.pop_time) c.flying = true;
            ++it;
        } else {
            sf::Vector2f d = coin_target_ - c.pos;
            float dist = std::sqrt(d.x * d.x + d.y * d.y);
            float speed = 500.0f + 1800.0f * (c.age - c.pop_time);
            if (dist < speed * dt || dist < 18.0f) {
                if (audio_) {
                    audio_->play(Sfx::COIN, 45.0f,
                                 1.0f + 0.06f * static_cast<float>(c.pitch_step % 10));
                }
                it = coins_.erase(it);
            } else {
                c.pos += d * (speed * dt / dist);
                ++it;
            }
        }
    }

    for (auto& tp : trail_particles_) tp.age += dt;
    trail_particles_.erase(std::remove_if(trail_particles_.begin(), trail_particles_.end(),
        [](const TrailParticle& tp) { return tp.age >= tp.lifetime; }),
        trail_particles_.end());

    if (screen_flash_ > 0.0f) screen_flash_ -= dt;
}

void EffectsLayer::spawnIncomeCoins(const std::vector<IncomeEvent>& events,
                                    sf::Vector2f counter_px) {
    coin_target_ = counter_px;
    int step = 0;
    for (const auto& ev : events) {
        int n = std::clamp(ev.amount / 3, 3, 10);
        sf::Vector2f origin = ui::cellCenter(ev.x, ev.y);
        for (int i = 0; i < n; ++i) {
            Coin c;
            c.pos        = origin;
            c.vel        = { frand(-70.0f, 70.0f), frand(-180.0f, -100.0f) };
            c.pop_time   = frand(0.35f, 0.6f);
            c.pitch_step = step++;
            coins_.push_back(c);
        }
    }
}

// ---------------------------------------------------------------------------
// Randare
// ---------------------------------------------------------------------------

void EffectsLayer::renderTrails(sf::RenderWindow& window) const {
    // Dara de foc pe drum: glow aditiv portocaliu care paleste.
    for (const auto& tp : trail_particles_) {
        float t = std::min(1.0f, tp.age / tp.lifetime);
        sf::Uint8 alpha = static_cast<sf::Uint8>(160.0f * (1.0f - t));
        drawGlow(window, sprites_.glowTex, tp.pos, 18.0f,
                 sf::Color(255, 140, 50, alpha));
    }
}

void EffectsLayer::renderForeground(sf::RenderWindow& window, const Game& game) const {
    // 1) Statusuri pe inamici (sub proiectile): vulnerabilitate, stun, ardere.
    for (const auto& e : game.getCurrentWave().getActiveEnemies()) {
        if (!e.isAlive()) continue;
        sf::Vector2f c = ui::cellCenter(e.getX(), e.getY());
        float ds = Sprites::displayScale(e.getName());

        if (e.isBurning()) {
            float flicker = 0.8f + 0.2f * std::sin(elapsed_ * 18.0f + e.id());
            drawGlow(window, sprites_.glowTex, c, ui::CELL_PX * 0.9f * ds,
                     sf::Color(255, 120, 30, static_cast<sf::Uint8>(90.0f * flicker)));
        }
        if (e.isVulnerable()) {
            float pulse = 0.42f + 0.03f * std::sin(elapsed_ * 6.0f);
            drawRing(window, c, ui::CELL_PX * pulse * ds, 2.5f,
                     sf::Color(200, 90, 255, 200), false);
        }
        if (e.isStunned()) {
            for (int i = 0; i < 3; ++i) {
                float ang = elapsed_ * 7.0f + i * 2.09f;
                sf::Vector2f p { c.x + std::cos(ang) * 14.0f,
                                 c.y - ui::CELL_PX * 0.45f * ds + std::sin(ang * 1.7f) * 4.0f };
                drawGlow(window, sprites_.glowTex, p, 9.0f, sf::Color(255, 255, 160, 220));
            }
        }
    }

    // 2) Proiectilele reale (din Wave): halo aditiv + nucleu solid.
    for (const auto& s : game.getCurrentWave().getShots()) {
        if (s.delay > 0.0f) continue;
        sf::Vector2f pos = ui::cellCenter(s.x, s.y);

        float r = 4.0f + std::min(5.0f, s.damage * 0.05f);
        if (s.style_mask & shotstyle::BRUISER) r *= 1.3f;

        drawGlow(window, sprites_.glowTex, pos, r * 5.0f,
                 sf::Color(baseShotColor(s).r, baseShotColor(s).g,
                           baseShotColor(s).b, 140));

        sf::CircleShape dot(r);
        dot.setFillColor(baseShotColor(s));
        dot.setOutlineColor(sf::Color::Black);
        dot.setOutlineThickness(1.0f);
        dot.setPosition(pos.x - r, pos.y - r);
        window.draw(dot);

        if (s.style_mask & shotstyle::PHOENIX) {
            float pr = r + 3.0f + 2.0f * std::sin(elapsed_ * 12.0f);
            drawRing(window, pos, pr, 2.5f, sf::Color(255, 120, 30, 230));
        }
        if (s.style_mask & shotstyle::BRUISER) {
            float phase = std::fmod(elapsed_ * 2.0f, 1.0f);
            drawRing(window, pos, r + 2.0f + 7.0f * phase, 2.0f,
                     sf::Color(255, 80, 80,
                               static_cast<sf::Uint8>(200.0f * (1.0f - phase))));
        }
        if (s.style_mask & shotstyle::DOUBLE_SHOT) {
            drawRing(window, pos, r + 2.0f, 1.5f, sf::Color(255, 210, 60, 220));
            drawRing(window, pos, r + 5.0f, 1.5f, sf::Color(255, 210, 60, 160));
        }
        if (s.style_mask & shotstyle::KNOCKBACK) {
            drawRing(window, pos, r + 3.0f, 3.0f, sf::Color(230, 140, 40, 220));
        }
        if (s.style_mask & shotstyle::SPLASH) {
            drawRing(window, pos, r + 4.0f, 2.0f, sf::Color(255, 180, 90, 180));
        }
        if (s.style_mask & shotstyle::EPIC) {
            drawRing(window, pos, r + 2.0f, 1.5f, sf::Color(160, 80, 200, 200));
        }
    }

    // 3) Orbitele de embers Phoenix: counter-ul de firestorm, vizibil ca
    // gameplay info SI ca spectacol (la 10 explodeaza).
    for (const auto& t : game.getTowers()) {
        const auto* pt = dynamic_cast<const ProjectileTower*>(t.get());
        if (!pt || !pt->mythicBadge()) continue;
        auto [vc, vr] = pt->visualPosition();
        sf::Vector2f c = ui::cellCenter(vc, vr);

        if (std::string("PhoenixBarrage") == pt->mythicBadge()) {
            int count = std::min(pt->emberCount(), 10);
            for (int i = 0; i < count; ++i) {
                float ang = elapsed_ * 1.6f + i * 0.6283f;
                sf::Vector2f p { c.x + std::cos(ang) * ui::CELL_PX * 0.62f,
                                 c.y + std::sin(ang) * ui::CELL_PX * 0.62f };
                float pulse = 0.8f + 0.2f * std::sin(elapsed_ * 9.0f + i);
                drawGlow(window, sprites_.glowTex, p, 12.0f * pulse,
                         sf::Color(255, 140, 40, 230));
            }
        } else {   // RovingBruiser: trei sateliti rosii care orbiteaza strans
            for (int i = 0; i < 3; ++i) {
                float ang = elapsed_ * 2.2f + i * 2.094f;
                sf::Vector2f p { c.x + std::cos(ang) * ui::CELL_PX * 0.5f,
                                 c.y + std::sin(ang) * ui::CELL_PX * 0.5f };
                drawGlow(window, sprites_.glowTex, p, 10.0f, sf::Color(255, 90, 90, 200));
            }
        }
    }

    // 4) Particule (aditive): impacturi, morti, foc, muzzle flash.
    for (const auto& p : particles_) {
        float t = std::min(1.0f, p.age / p.lifetime);
        float size = p.size * 2.2f * (1.0f - t * 0.6f);
        sf::Uint8 alpha = static_cast<sf::Uint8>(230.0f * (1.0f - t));
        drawGlow(window, sprites_.glowTex, p.pos, size,
                 sf::Color(p.color.r, p.color.g, p.color.b, alpha));
    }

    // 5) Inele in expansiune (shockwave, firestorm, boss).
    for (const auto& r : rings_) {
        float t = std::min(1.0f, r.age / r.lifetime);
        float ease = 1.0f - (1.0f - t) * (1.0f - t);   // pleaca rapid, incetineste
        float radius = r.r0 + (r.r1 - r.r0) * ease;
        sf::Uint8 alpha = static_cast<sf::Uint8>(220.0f * (1.0f - t));
        drawRing(window, r.pos, radius, r.thickness * (1.0f - 0.5f * t),
                 sf::Color(r.color.r, r.color.g, r.color.b, alpha));
    }

    // 6) Monede de income: glow auriu + nucleu solid.
    for (const auto& c : coins_) {
        drawGlow(window, sprites_.glowTex, c.pos, 20.0f, sf::Color(255, 200, 80, 160));
        sf::CircleShape core(5.0f);
        core.setFillColor(sf::Color(255, 215, 90));
        core.setOutlineColor(sf::Color(120, 80, 10));
        core.setOutlineThickness(1.5f);
        core.setPosition(c.pos.x - 5.0f, c.pos.y - 5.0f);
        window.draw(core);
    }

    // 7) Texte plutitoare: damage numbers colorate + "+$X" la kill.
    if (font_) {
        for (const auto& ft : texts_) {
            float t = ft.age / ft.lifetime;
            sf::Uint8 alpha = (t < 0.5f)
                ? 255
                : static_cast<sf::Uint8>(255.0f * (1.0f - (t - 0.5f) * 2.0f));
            sf::Text txt;
            txt.setFont(*font_);
            txt.setString(ft.text);
            txt.setCharacterSize(ft.size);
            txt.setFillColor(sf::Color(ft.color.r, ft.color.g, ft.color.b, alpha));
            txt.setOutlineColor(sf::Color(0, 0, 0, alpha));
            txt.setOutlineThickness(2.0f);
            sf::FloatRect b = txt.getLocalBounds();
            txt.setOrigin(b.left + b.width / 2.0f, b.top + b.height);
            txt.setPosition(ft.pos);
            window.draw(txt);
        }
    }

    // 8) Flash pe tot ecranul (firestorm / boss kill), aditiv.
    if (screen_flash_ > 0.0f) {
        float t = std::min(1.0f, screen_flash_ / 0.45f);
        sf::RectangleShape flash({static_cast<float>(ui::WIN_W),
                                  static_cast<float>(ui::WIN_H)});
        flash.setFillColor(sf::Color(flash_color_.r, flash_color_.g, flash_color_.b,
                                     static_cast<sf::Uint8>(70.0f * t)));
        window.draw(flash, sf::RenderStates(sf::BlendAdd));
    }
}
