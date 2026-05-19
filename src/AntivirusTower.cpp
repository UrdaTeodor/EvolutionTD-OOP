#include "AntivirusTower.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>


AntivirusTower::AntivirusTower(int col, int row)
    : Tower("PrimitiveAV", 50, col, row, 4.0f),
      damage(25.0f), attackSpeed(1.0f), projectileSpeed(8.0f), attackCooldown(0.0f),
      doubleShot(false), fireTrail(false), multiTarget(false),
      knockbackInterval(0), shotCounter(0) {}

std::unique_ptr<Tower> makeAntivirus(int col, int row) {
    return std::make_unique<AntivirusTower>(col, row);
}

bool AntivirusTower::isInRange(const Enemy& enemy) const {
    float dx = enemy.getX() - static_cast<float>(getX());
    float dy = enemy.getY() - static_cast<float>(getY());
    return std::sqrt(dx * dx + dy * dy) <= getRange();
}

std::pair<float, float> AntivirusTower::calculateInterceptPoint(const Enemy& enemy) const {
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

void AntivirusTower::attackEnemy(Enemy& enemy) {
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

void AntivirusTower::update(std::vector<Enemy>& enemies, float deltaTime) {
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
        // TODO: cand abilitatile sunt active (doubleShot, fireTrail, etc.),
        // Placeholder.
        ++shotCounter;
    }
}

char AntivirusTower::getDisplayChar() const { return 'A'; }

std::unique_ptr<Tower> AntivirusTower::clone() const {
    return std::make_unique<AntivirusTower>(*this);
}

// stat buffs (apelate din StatEvolution::apply prin dynamic_cast)
void AntivirusTower::buffDamage(float pct)      { damage      *= (1.0f + pct); }
void AntivirusTower::buffAttackSpeed(float pct) { attackSpeed *= (1.0f + pct); }

// evo enablers (apelate din AbilityEvolution::apply) 
void AntivirusTower::enableDoubleShot()                { doubleShot = true; }
void AntivirusTower::enableFireTrail()                 { fireTrail = true; }
void AntivirusTower::enableMultiTarget()               { multiTarget = true; }
void AntivirusTower::enableKnockback(int interval)     { knockbackInterval = interval; }

void AntivirusTower::displayDetails(std::ostream& os) const {
    os << " dmg:" << damage << " aspd:" << attackSpeed;
    if (doubleShot)            os << " [DoubleShot]";
    if (fireTrail)             os << " [FireTrail]";
    if (multiTarget)           os << " [MultiTarget]";
    if (knockbackInterval > 0) os << " [Knockback/" << knockbackInterval << "]";
}
