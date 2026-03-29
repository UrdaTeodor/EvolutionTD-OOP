#include "Enemy.h"
#include <cmath>
#include <iostream>


Enemy makeAdware() { return Enemy("Adware", 50.0f,  2.0f, 10); }
Enemy makeTrojan() { return Enemy("Trojan", 150.0f, 1.0f, 25); }
Enemy makeWorm()   { return Enemy("Worm",   30.0f,  4.0f, 15); }

//constructor

Enemy::Enemy(const std::string& name, float maxHealth, float speed, int reward)
    : name(name), maxHealth(maxHealth), currentHealth(maxHealth),
      speed(speed), reward(reward),
      pathIndex(1), x(0.0f), y(0.0f),
      slowFactor(1.0f), vx(0.0f), vy(0.0f) {}

// calculeaza distanta pana la un punct

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

void Enemy::move(const std::vector<std::pair<int, int>>& path, float deltaTime) {
    if (pathIndex >= static_cast<int>(path.size())) return; // path.size returneaza nr de waypoints(start + 2 curbe + end) iar pathIndex are indexul urmatorului waypoint, deci daca e >= inseamna ca am trecut de ultimul waypoint si am ajuns la final

    float targetX = static_cast<float>(path[pathIndex].second); // .second = col = poz orizontala
    float targetY = static_cast<float>(path[pathIndex].first);  // .first  = row = poz verticala
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
        float nextX   = static_cast<float>(path[pathIndex].second); 
        float nextY   = static_cast<float>(path[pathIndex].first);  
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

bool Enemy::isAlive() const {
    return currentHealth > 0.0f;
}

bool Enemy::hasReachedEnd(const std::vector<std::pair<int, int>>& path) const {
    return pathIndex >= static_cast<int>(path.size());
}

float Enemy::getX()             const { return x; }
float Enemy::getY()             const { return y; }
float Enemy::getCurrentHealth() const { return currentHealth; }
float Enemy::getMaxHealth()     const { return maxHealth; }
float Enemy::getEffectiveSpeed() const { return speed * slowFactor; }
float Enemy::getVelocityX()     const { return vx; }
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
