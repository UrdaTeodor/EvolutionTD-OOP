#include "Wave.h"
#include "GlobalStatBuffs.h"
#include "HoneypotTower.h"
#include "ProjectileTower.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace {
    Enemy* findEnemyById(std::vector<Enemy>& enemies, int id) {
        for (auto& e : enemies) {
            if (e.id() == id) return &e;
        }
        return nullptr;
    }
}

//Regula celor 3 (cand Wave va contine pointeri la inamici polimorfici)


Wave::Wave(int waveNumber, std::vector<Enemy> enemies)
    : activeEnemies(), pendingEnemies(enemies),
      waveNumber(waveNumber), spawnTimer(0.0f) {}

Wave::Wave(const Wave& other)
    : activeEnemies(other.activeEnemies),
      pendingEnemies(other.pendingEnemies),
      shots_(other.shots_),
      visual_events_(other.visual_events_),
      waveNumber(other.waveNumber),
      spawnTimer(other.spawnTimer),
      spawn_interval_(other.spawn_interval_),
      boss_escaped_(other.boss_escaped_),
      final_boss_wave_(other.final_boss_wave_) {}


Wave& Wave::operator=(const Wave& other) {
    if (this != &other) {
        activeEnemies   = other.activeEnemies;
        pendingEnemies  = other.pendingEnemies;
        shots_          = other.shots_;
        visual_events_  = other.visual_events_;
        waveNumber      = other.waveNumber;
        spawnTimer      = other.spawnTimer;
        spawn_interval_ = other.spawn_interval_;
        boss_escaped_   = other.boss_escaped_;
        final_boss_wave_ = other.final_boss_wave_;
    }
    return *this;
}


Wave::~Wave() {}

// towers e vector de unique_ptr<Tower> => apel polimorfic prin tower->update().
// cppcheck-suppress unusedFunction
int Wave::simulate(std::vector<std::unique_ptr<Tower>>& towers,
                   const std::vector<std::pair<int, int>>& path,
                   float deltaTime,
                   int& moneyEarned,
                   int& killedCount,
                   const GlobalStatBuffs& buffs) {
    moneyEarned = 0;
    killedCount = 0;

    //  Genereaza urmatorul inamic din coada cand timerul expira.
    // Spawn FIFO
    spawnTimer -= deltaTime;
    if (spawnTimer <= 0.0f && !pendingEnemies.empty()) {
        Enemy spawned = pendingEnemies.front();
        pendingEnemies.erase(pendingEnemies.begin());
        spawned.placeAt(static_cast<float>(path[0].second),
                        static_cast<float>(path[0].first));
        activeEnemies.push_back(spawned);
        spawnTimer = spawn_interval_;
    }

    //  Reseteaza incetinirea pe toti inamicii; turnurile Honeypot o vor reaplica
    for (auto& enemy : activeEnemies) {
        enemy.resetSlow();
    }

    // Predicted death: recalculeaza damage-ul in zbor per inamic, ca turnurile
    // sa nu mai traga in tinte deja condamnate (overkill = fizzle irosit).
    for (auto& enemy : activeEnemies) enemy.setIncomingDamage(0.0f);
    for (const auto& s : shots_) {
        if (Enemy* t = findEnemyById(activeEnemies, s.target_id)) {
            t->addIncomingDamage(s.damage);
        }
    }

    // Honeypot-urile de SUPORT (AMPLIFY/OVERCLOCK) buff-uiesc turnurile din
    // aura. Per stat se ia MAXIMUL dintre honeypot-uri (nu se aduna — acelasi
    // pattern anti-stack ca vulnerabilitatea); stack-urile pe ACELASI honeypot
    // se aduna prin getteri. Recalculat per tick: muti turnul, dispare buff-ul.
    for (auto& tower : towers) {
        float dmg_pct = 0.0f, aspd_pct = 0.0f;
        for (const auto& other : towers) {
            const auto* hp = dynamic_cast<const HoneypotTower*>(other.get());
            if (!hp || !hp->isSupport() || hp == tower.get()) continue;
            float hdx = static_cast<float>(tower->getX() - hp->getX());
            float hdy = static_cast<float>(tower->getY() - hp->getY());
            if (std::sqrt(hdx * hdx + hdy * hdy) <= hp->auraRange(buffs)) {
                dmg_pct  = std::max(dmg_pct,  hp->supportDamagePct());
                aspd_pct = std::max(aspd_pct, hp->supportAttackSpeedPct());
            }
        }
        tower->setLocalBuffs(dmg_pct, aspd_pct);
    }

    // Turnurile lanseaza proiectile (damage-ul se aplica la impact, mai jos).
    const size_t shots_before = shots_.size();
    for (auto& tower : towers) {
        tower->update(activeEnemies, deltaTime, buffs, path, shots_);
    }
    // Focurile noi din acest tick -> evenimente SHOT_FIRED (recoil/flash/sunet).
    for (size_t i = shots_before; i < shots_.size(); ++i) {
        const Shot& s = shots_[i];
        VisualEvent ev;
        ev.type            = VisualEvent::Type::SHOT_FIRED;
        ev.x               = s.x;
        ev.y               = s.y;
        ev.style_mask      = s.style_mask;
        ev.source_tower_id = s.source_tower_id;
        ev.source_char     = s.source_char;
        ev.delay           = s.delay;
        visual_events_.push_back(ev);
    }

    tickShots(towers, path, deltaTime);

    // DoT FIRE_TRAIL: aplicat per-frame pe inamicii cu fire_trail_remaining > 0.
    for (auto& enemy : activeEnemies) {
        if (enemy.isAlive()) enemy.tickFireTrail(deltaTime);
    }

    // Misca fiecare inamic
    for (auto& enemy : activeEnemies) {
        if (enemy.isAlive()) {
            enemy.move(path, deltaTime);
        }
    }

    // inamici ucisi -> bani, inamici scapati -> hp dmg
    int playerDamage = 0;
    std::vector<Enemy> survivors;
    for (const auto& enemy : activeEnemies) {
        if (!enemy.isAlive()) {
            moneyEarned += enemy.getReward();
            killedCount += 1;
            // DEATH prinde orice cauza (proiectil sau DoT): emis la scoaterea
            // din simulare, deci exact o data per inamic.
            VisualEvent ev;
            ev.type       = VisualEvent::Type::DEATH;
            ev.x          = enemy.getX();
            ev.y          = enemy.getY();
            ev.enemy_name = enemy.getName();
            ev.reward     = enemy.getReward();
            visual_events_.push_back(ev);
        } else if (enemy.hasReachedEnd(path)) {
            // Leak damage fix per tip de inamic (din EnemySpec), nu HP-ul curent:
            // cu HP scalat pe valuri orice scapare ar fi fost fatala instant.
            int dmg = enemy.getLeakDamage();
            // Doar boss-ul SCRIPTAT (valul final) e defeat instant; ILOVEYOU
            // de Director (endless) face leak normal din spec.
            if (enemy.getName() == "ILOVEYOU" && final_boss_wave_) {
                boss_escaped_ = true;
                dmg = 99999;
            }
            playerDamage += dmg;
        } else {
            survivors.push_back(enemy);
        }
    }
    activeEnemies = survivors;

    return playerDamage;
}

// Proiectile homing: urmaresc tinta dupa id; daca tinta a murit/a iesit,
// proiectilul dispare fara efect (fizzle) — overkill-ul e risipa reala.
void Wave::tickShots(std::vector<std::unique_ptr<Tower>>& towers,
                     const std::vector<std::pair<int, int>>& path,
                     float deltaTime) {
    for (auto it = shots_.begin(); it != shots_.end();) {
        Shot& s = *it;

        // Rafala DoubleShot: focul asteapta delay-ul inainte sa plece.
        if (s.delay > 0.0f) {
            s.delay -= deltaTime;
            ++it;
            continue;
        }

        Enemy* target = findEnemyById(activeEnemies, s.target_id);
        if (!target || !target->isAlive()) {
            it = shots_.erase(it);   // fizzle
            continue;
        }

        float dx   = target->getX() - s.x;
        float dy   = target->getY() - s.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        float step = s.speed * deltaTime;

        if (step < dist) {
            s.x += dx / dist * step;
            s.y += dy / dist * step;
            ++it;
            continue;
        }

        // IMPACT: aici se aplica damage-ul si toate efectele.
        bool was_burning = target->isBurning();
        target->takeDamage(s.damage);

        VisualEvent ev;
        ev.type       = VisualEvent::Type::IMPACT;
        ev.x          = target->getX();
        ev.y          = target->getY();
        ev.style_mask = s.style_mask;
        ev.damage     = s.damage;
        ev.killed     = !target->isAlive();
        ev.knockback  = s.knockback;
        ev.shockwave  = s.knockback && s.shockwave;
        if (dist > 1e-4f) {
            ev.dir_x = dx / dist;
            ev.dir_y = dy / dist;
        }
        visual_events_.push_back(ev);

        if (s.fire_dps > 0.0f) target->applyFireTrail(s.fire_dps, 3.0f);
        if (s.excess_slow)     target->applyTimedSlow(0.5f, 1.0f);
        if (s.knockback) {
            target->pushBack(s.knock_cells, path);
            // RovingBruiser: unda de soc — push + vulnerabilitate in jurul tintei.
            if (s.shockwave) {
                target->applyVulnerability(3.0f);
                for (auto& e : activeEnemies) {
                    if (!e.isAlive() || &e == target) continue;
                    float sdx = e.getX() - target->getX();
                    float sdy = e.getY() - target->getY();
                    if (std::sqrt(sdx * sdx + sdy * sdy) <= 1.5f) {
                        e.pushBack(s.knock_cells, path);
                        e.applyVulnerability(3.0f);
                    }
                }
            }
        }

        // PhoenixBarrage: kill pe un inamic care ardea => ember pentru turnul-sursa.
        if ((s.style_mask & shotstyle::PHOENIX) && was_burning && !target->isAlive()) {
            for (auto& t : towers) {
                if (t->instanceId() == s.source_tower_id) {
                    if (auto* pt = dynamic_cast<ProjectileTower*>(t.get())) {
                        pt->addEmber();
                    }
                    break;
                }
            }
        }

        // BlastWave: damage AoE in jurul tintei (doar damage, fara foc/push —
        // efectele speciale raman pe lovitura directa).
        if (s.splash_pct > 0.0f) {
            constexpr float SPLASH_RADIUS = 1.2f;
            for (auto& e : activeEnemies) {
                if (!e.isAlive() || &e == target) continue;
                float sdx = e.getX() - target->getX();
                float sdy = e.getY() - target->getY();
                if (std::sqrt(sdx * sdx + sdy * sdy) <= SPLASH_RADIUS) {
                    e.takeDamage(s.damage * s.splash_pct);
                }
            }
        }

        // Pierce clasic (DoubleShot): proiectilul STRAPUNGE tinta si continua
        // spre un inamic NOU din spatele ei, pe directia de zbor (con ~60°,
        // max 2.5 celule), cu damage injumatatit. Knockback-ul/slow-ul sunt
        // consumate de prima lovitura; focul ramane (aprinde tot ce strapunge).
        if (s.pierce_left > 0 && dist > 1e-4f) {
            float ndx = dx / dist;
            float ndy = dy / dist;
            Enemy* next = nullptr;
            float bestD = 2.5f;
            for (auto& e : activeEnemies) {
                if (!e.isAlive() || e.id() == s.target_id || e.isPredictedDead()) continue;
                float ex = e.getX() - target->getX();
                float ey = e.getY() - target->getY();
                float d  = std::sqrt(ex * ex + ey * ey);
                if (d < 1e-4f || d > bestD) continue;
                if ((ex * ndx + ey * ndy) / d < 0.5f) continue;   // nu e "in fata"
                bestD = d;
                next  = &e;
            }
            if (next) {
                --s.pierce_left;
                s.damage *= 0.5f;
                s.target_id   = next->id();
                s.x           = target->getX();
                s.y           = target->getY();
                s.knockback   = false;
                s.shockwave   = false;
                s.excess_slow = false;
                next->addIncomingDamage(s.damage);
                ++it;          // proiectilul zboara mai departe
                continue;
            }
        }

        it = shots_.erase(it);
    }
}

void Wave::addEnemy(const Enemy& enemy) {
    pendingEnemies.push_back(enemy);
}

std::vector<VisualEvent> Wave::takeVisualEvents() {
    std::vector<VisualEvent> out;
    out.swap(visual_events_);
    return out;
}

// cppcheck-suppress unusedFunction
bool Wave::allDefeated() const {
    return pendingEnemies.empty() && activeEnemies.empty();
}

// cppcheck-suppress unusedFunction
const std::vector<Enemy>& Wave::getActiveEnemies() const {
    return activeEnemies;
}

// Friend operator<<: poate accesa direct membrii private ai Wave.
std::ostream& operator<<(std::ostream& os, const Wave& w) {
    os << "Wave " << w.waveNumber
       << " | active:" << w.activeEnemies.size()
       << " | queued:" << w.pendingEnemies.size() << "\n";
    for (const auto& e : w.activeEnemies) {
        os << "  " << e << "\n";  // -> Enemy::operator<< (src/Enemy.cpp)
    }
    return os;
}
