#include "AdblockerTower.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

AdblockerTower::AdblockerTower(int col, int row)
    : Tower("Adblocker", 40, col, row, 3.0f),
      damage(8.0f), attackSpeed(4.0f), projectileSpeed(10.0f), attackCooldown(0.0f),
      doubleShot(false), fireTrail(false), multiTarget(false),
      knockbackInterval(0), shotCounter(0) {}

std::unique_ptr<Tower> makeAdblocker(int col, int row) {
    return std::make_unique<AdblockerTower>(col, row);
}

bool AdblockerTower::isInRange(const Enemy& enemy) const {
    float dx = enemy.getX() - static_cast<float>(getX());
    float dy = enemy.getY() - static_cast<float>(getY());
    return std::sqrt(dx * dx + dy * dy) <= getRange();
}

std::pair<float, float> AdblockerTower::calculateInterceptPoint(const Enemy& enemy) const {
    float ex = enemy.getX();
    float ey = enemy.getY();
    float vx = enemy.getVelocityX();
    float vy = enemy.getVelocityY();
    float ps = projectileSpeed;

    float dx = ex - static_cast<float>(getX());
    float dy = ey - static_cast<float>(getY());

    float a = vx * vx + vy * vy - ps * ps;
    float b = 2.0f * (dx * vx + dy * vy);
    float c = dx * dx + dy * dy;

    float t = 0.0f;

    if (std::abs(a) < 0.0001f) {
        t = (std::abs(b) > 0.0001f) ? (-c / b) : 0.0f;
    } else {
        float disc = b * b - 4.0f * a * c;
        if (disc < 0.0f) return {ex, ey};
        float sqrtDisc = std::sqrt(disc);
        float t1 = (-b + sqrtDisc) / (2.0f * a);
        float t2 = (-b - sqrtDisc) / (2.0f * a);
        if      (t1 > 0.0f && t2 > 0.0f) t = std::min(t1, t2);
        else if (t1 > 0.0f)               t = t1;
        else if (t2 > 0.0f)               t = t2;
        else return {ex, ey};
    }

    return {ex + vx * t, ey + vy * t};
}

void AdblockerTower::attackEnemy(Enemy& enemy) {
    std::pair<float, float> target = calculateInterceptPoint(enemy);
    float ix = target.first;
    float iy = target.second;
    std::cout << "  " << getName() << " fires -> predicted pos ("
              << static_cast<int>(ix) << "," << static_cast<int>(iy) << ")  "
              << enemy.getName() << " HP: " << static_cast<int>(enemy.getCurrentHealth())
              << " -> ";
    enemy.takeDamage(damage);
    std::cout << static_cast<int>(enemy.getCurrentHealth()) << "\n";
}

void AdblockerTower::update(std::vector<Enemy>& enemies, float deltaTime) {
    attackCooldown -= deltaTime;
    if (attackCooldown > 0.0f) return;

    Enemy* target = nullptr;
    float minDist = std::numeric_limits<float>::max();
    for (auto& enemy : enemies) {
        if (!enemy.isAlive() || !isInRange(enemy)) continue;
        float dx = enemy.getX() - static_cast<float>(getX());
        float dy = enemy.getY() - static_cast<float>(getY());
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist < minDist) {
            minDist = dist;
            target = &enemy;
        }
    }

    if (target) {
        attackEnemy(*target);
        attackCooldown = 1.0f / attackSpeed;
        ++shotCounter;
    }
}

char AdblockerTower::getDisplayChar() const { return 'D'; }

std::unique_ptr<Tower> AdblockerTower::clone() const {
    return std::make_unique<AdblockerTower>(*this);
}

void AdblockerTower::buffDamage(float pct)      { damage      *= (1.0f + pct); }
void AdblockerTower::buffAttackSpeed(float pct) { attackSpeed *= (1.0f + pct); }

void AdblockerTower::enableDoubleShot()                { doubleShot = true; }
void AdblockerTower::enableFireTrail()                 { fireTrail = true; }
void AdblockerTower::enableMultiTarget()               { multiTarget = true; }
void AdblockerTower::enableKnockback(int interval)     { knockbackInterval = interval; }

void AdblockerTower::displayDetails(std::ostream& os) const {
    os << " dmg:" << damage << " aspd:" << attackSpeed;
    if (doubleShot)            os << " [DoubleShot]";
    if (fireTrail)             os << " [FireTrail]";
    if (multiTarget)           os << " [MultiTarget]";
    if (knockbackInterval > 0) os << " [Knockback/" << knockbackInterval << "]";
}
