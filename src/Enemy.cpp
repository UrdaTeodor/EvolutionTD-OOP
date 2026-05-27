#include "Enemy.h"
#include "EnemySpec.h"
#include <algorithm>
#include <cmath>
#include <iostream>


// pathIndex = 1 pentru ca pozitia 0 din path e startul
Enemy::Enemy(const std::string& name, float maxHealth, float speed, int reward)
    : name(name), maxHealth(maxHealth), currentHealth(maxHealth),
      speed(speed), reward(reward),
      pathIndex(1), x(0.0f), y(0.0f),
      slowFactor(1.0f), vx(0.0f), vy(0.0f) {}

// T3 constructor din spec
Enemy::Enemy(const EnemySpec& spec)
    : Enemy(spec.display_name, spec.max_health, spec.speed, spec.reward) {}


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
    currentHealth -= damage;
    if (currentHealth < 0.0f) currentHealth = 0.0f;
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
    if (cells <= 0 || path.empty()) return;
    // pathIndex e index-ul waypoint-ului URMATOR. Scadem cu cells (clamp la 1).
    pathIndex -= cells;
    if (pathIndex < 1) pathIndex = 1;
    // Plasam exact la waypoint-ul anterior (path[pathIndex-1]).
    x = static_cast<float>(path[pathIndex - 1].second);
    y = static_cast<float>(path[pathIndex - 1].first);
    // Recalculam vx/vy spre noul target (waypoint pathIndex).
    if (pathIndex < static_cast<int>(path.size())) {
        float nextX = static_cast<float>(path[pathIndex].second);
        float nextY = static_cast<float>(path[pathIndex].first);
        float dist  = distanceTo(nextX, nextY);
        if (dist > 0.01f) {
            float spd = getEffectiveSpeed();
            vx = (nextX - x) / dist * spd;
            vy = (nextY - y) / dist * spd;
        }
    }
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
float Enemy::getEffectiveSpeed() const { return speed * slowFactor; }
// cppcheck-suppress unusedFunction
float Enemy::getVelocityX()     const { return vx; }
// cppcheck-suppress unusedFunction
float Enemy::getVelocityY()     const { return vy; }
const std::string& Enemy::getName() const { return name; }
int Enemy::getReward()          const { return reward; }

std::ostream& operator<<(std::ostream& os, const Enemy& e) {
    os << "[" << e.name
       << " HP:" << static_cast<int>(e.currentHealth) << "/" << static_cast<int>(e.maxHealth)
       << " spd:" << e.getEffectiveSpeed()
       << " pos:(" << static_cast<int>(e.x) << "," << static_cast<int>(e.y) << ")"
       << "]";
    return os;
}
