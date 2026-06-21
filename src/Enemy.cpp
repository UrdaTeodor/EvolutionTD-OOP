#include "Enemy.h"
#include "EnemySpec.h"
#include <algorithm>
#include <cmath>
#include <iostream>


int Enemy::next_id_ = 0;

// pathIndex = 1 pentru ca pozitia 0 din path e startul
Enemy::Enemy(const std::string& name, float maxHealth, float speed, int reward)
    : id_(next_id_++),
      name(name), maxHealth(maxHealth), currentHealth(maxHealth),
      speed(speed), reward(reward),
      pathIndex(1), x(0.0f), y(0.0f),
      slowFactor(1.0f), vx(0.0f), vy(0.0f) {}

// T3 constructor din spec
Enemy::Enemy(const EnemySpec& spec)
    : Enemy(spec.display_name, spec.max_health, spec.speed, spec.reward) {
    leakDamage = spec.leak_damage;
    isBoss_    = spec.is_boss;
}


float Enemy::distanceTo(float targetX, float targetY) const {
    float dx = targetX - x;
    float dy = targetY - y;
    return std::sqrt(dx * dx + dy * dy);
}


void Enemy::placeAt(float startX, float startY) {
    x = startX;
    y = startY;
    pathIndex = 1; // pathIndex este urmatorul waypoint (start finish si curbe)
    vx = 0.0f;
    vy = 0.0f;
}


// Cand ajunge la un waypoint, trece la urmatorul. Updateaza si viteza (vx, vy)

void Enemy::move(const std::vector<std::pair<int, int>>& path, float deltaTime) {
    // Statusurile expira pe masura ce trece timpul (tick aici, move e apelat
    // o data per frame pentru fiecare inamic viu).
    if (vulnerable_remaining_ > 0.0f) vulnerable_remaining_ -= deltaTime;
    if (tslow_remaining_ > 0.0f) {
        tslow_remaining_ -= deltaTime;
        if (tslow_remaining_ <= 0.0f) tslow_factor_ = 1.0f;
    }
    if (stun_remaining_ > 0.0f) {
        stun_remaining_ -= deltaTime;
        vx = vy = 0.0f;
        return;   // stunned: sta pe loc
    }

    // Knockback fizic: aluneca inapoi pe path pana consuma distanta.
    if (kb_dist_left_ > 0.0f) {
        moveBackward(kb_speed_ * deltaTime, path);
        vx = vy = 0.0f;
        return;
    }

    if (pathIndex >= static_cast<int>(path.size())) return;  // a trecut de ultimul waypoint

    float targetX = static_cast<float>(path[pathIndex].second); 
    float targetY = static_cast<float>(path[pathIndex].first);  
    float dist    = distanceTo(targetX, targetY);
    float step    = getEffectiveSpeed() * deltaTime;

    if (step >= dist) {
        // daca putem ajunge sau depasi tinta in acest pas, plasam exact la tinta si trecem la urmatorul waypoint
        x = targetX;
        y = targetY;
        pathIndex++;
    } else {
        // altfel, ne miscam cat putem in directia tinta
        x += (targetX - x) / dist * step;
        y += (targetY - y) / dist * step;
    }

    // updateaza vectorul de viteza
    if (pathIndex < static_cast<int>(path.size())) {
        float nextX = static_cast<float>(path[pathIndex].second);
        float nextY = static_cast<float>(path[pathIndex].first);
        float nextDist = distanceTo(nextX, nextY);
        if (nextDist > 0.01f) {
            float spd = getEffectiveSpeed();
            vx = (nextX - x) / nextDist * spd;
            vy = (nextY - y) / nextDist * spd;
        } else {
            vx = vy = 0.0f;
        }
    } else {
        vx = vy = 0.0f;
    }
}

void Enemy::takeDamage(float damage) {
    // RovingBruiser: inamicii loviti de unda de soc primesc +25% damage din
    // orice sursa cat timp dureaza vulnerabilitatea.
    if (vulnerable_remaining_ > 0.0f) damage *= 1.25f;
    currentHealth -= damage;
    if (currentHealth < 0.0f) currentHealth = 0.0f;
}

void Enemy::applyStun(float seconds) {
    stun_remaining_ = std::max(stun_remaining_, seconds);
}

void Enemy::applyVulnerability(float seconds) {
    vulnerable_remaining_ = std::max(vulnerable_remaining_, seconds);
}

void Enemy::applyTimedSlow(float factor, float seconds) {
    // Pastreaza cel mai puternic slow si cea mai lunga durata (refresh, nu stack).
    tslow_factor_    = std::min(tslow_factor_, factor);
    tslow_remaining_ = std::max(tslow_remaining_, seconds);
}

void Enemy::applySlow(float factor) {
    slowFactor = factor;
}

void Enemy::resetSlow() {
    slowFactor = 1.0f;
}

// cppcheck-suppress unusedFunction
void Enemy::scaleHealth(float multiplier) {
    if (multiplier <= 0.0f) return;
    maxHealth     *= multiplier;
    currentHealth  = maxHealth;
}

void Enemy::applyFireTrail(float dps, float duration_sec) {
    // Refresh in loc de stacking — daca user aplica fire trail repetat, ramane la max.
    fire_trail_dps_       = std::max(fire_trail_dps_, dps);
    fire_trail_remaining_ = std::max(fire_trail_remaining_, duration_sec);
}

void Enemy::tickFireTrail(float dt) {
    if (fire_trail_remaining_ <= 0.0f || fire_trail_dps_ <= 0.0f) return;
    float burn_dt = std::min(dt, fire_trail_remaining_);
    takeDamage(fire_trail_dps_ * burn_dt);
    fire_trail_remaining_ -= burn_dt;
    if (fire_trail_remaining_ <= 0.0f) {
        fire_trail_dps_       = 0.0f;
        fire_trail_remaining_ = 0.0f;
    }
}

void Enemy::pushBack(int cells, const std::vector<std::pair<int, int>>& path) {
    // Bosii sunt imuni: knockback stackat cu attack speed dadea stall infinit
    // (ILOVEYOU nu mai ajungea niciodata la baza).
    if (isBoss_) return;
    if (cells <= 0 || path.empty()) return;
    // Knockback FIZIC: acumuleaza distanta de parcurs inapoi; toata distanta
    // curenta se consuma in ~0.3s (vezi cu ochii cum zboara inamicul).
    kb_dist_left_ += static_cast<float>(cells);
    kb_speed_      = kb_dist_left_ / 0.3f;
}

void Enemy::moveBackward(float step, const std::vector<std::pair<int, int>>& path) {
    step = std::min(step, kb_dist_left_);
    kb_dist_left_ -= step;

    while (step > 0.0f) {
        if (pathIndex <= 1) {
            // Clamp la spawn: nu poate iesi din harta prin spate.
            x = static_cast<float>(path[0].second);
            y = static_cast<float>(path[0].first);
            kb_dist_left_ = 0.0f;
            break;
        }
        // Tinta inapoi = waypoint-ul anterior (pathIndex - 1).
        float bx = static_cast<float>(path[pathIndex - 1].second);
        float by = static_cast<float>(path[pathIndex - 1].first);
        float d  = distanceTo(bx, by);
        if (step >= d) {
            x = bx;
            y = by;
            --pathIndex;
            step -= d;
        } else {
            x += (bx - x) / d * step;
            y += (by - y) / d * step;
            step = 0.0f;
        }
    }
    if (kb_dist_left_ <= 0.0f) {
        kb_dist_left_ = 0.0f;
        kb_speed_     = 0.0f;
    }
}

float Enemy::pathProgress(const std::vector<std::pair<int, int>>& path) const {
    if (pathIndex >= static_cast<int>(path.size())) return 1e9f;   // practic la baza
    float nx = static_cast<float>(path[pathIndex].second);
    float ny = static_cast<float>(path[pathIndex].first);
    return static_cast<float>(pathIndex) * 1000.0f - distanceTo(nx, ny);
}

bool Enemy::isAlive() const {
    return currentHealth > 0.0f;
}

bool Enemy::hasReachedEnd(const std::vector<std::pair<int, int>>& path) const {
    return pathIndex >= static_cast<int>(path.size());
}

float Enemy::getX()             const { return x; }
float Enemy::getY()             const { return y; }
float Enemy::getCurrentHealth() const { return currentHealth; }
// cppcheck-suppress unusedFunction
float Enemy::getMaxHealth()     const { return maxHealth; }
float Enemy::getEffectiveSpeed() const {
    float t = (tslow_remaining_ > 0.0f) ? tslow_factor_ : 1.0f;
    return speed * slowFactor * t;
}
// cppcheck-suppress unusedFunction
float Enemy::getVelocityX()     const { return vx; }
// cppcheck-suppress unusedFunction
float Enemy::getVelocityY()     const { return vy; }
const std::string& Enemy::getName() const { return name; }
int Enemy::getReward()          const { return reward; }
int Enemy::getLeakDamage()      const { return leakDamage; }

std::ostream& operator<<(std::ostream& os, const Enemy& e) {
    os << "[" << e.name
       << " HP:" << static_cast<int>(e.currentHealth) << "/" << static_cast<int>(e.maxHealth)
       << " spd:" << e.getEffectiveSpeed()
       << " pos:(" << static_cast<int>(e.x) << "," << static_cast<int>(e.y) << ")"
       << "]";
    return os;
}
