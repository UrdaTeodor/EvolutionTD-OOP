#include "Tower.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

Tower::Tower(const std::string& name, TowerType type,
             float damage, float attackSpeed, float range,
             float projectileSpeed, int cost, int x, int y,
             float maxHP, float regenRate)
    : name(name), type(type),
      damage(damage), attackSpeed(attackSpeed),
      range(range), projectileSpeed(projectileSpeed),
      cost(cost), x(x), y(y),
      attackCooldown(0.0f),
      currentHP(maxHP), maxHP(maxHP), regenRate(regenRate) {}


Tower makeAntivirus(int col, int row) {
    // tower clasic default: 1 glont/s, raza 4
    return Tower("PrimitiveAV", TowerType::ANTIVIRUS, 25.0f, 1.0f, 4.0f, 8.0f, 50, col, row);
}

Tower makeAdblocker(int col, int row) {
    // trage rapid cu damage si range mai moc, overall DPS mai mare fata de AV
    return Tower("Adblocker", TowerType::ADBLOCKER, 8.0f, 4.0f, 3.0f, 10.0f, 40, col, row);
}

Tower makeHoneypot(int col, int row) {
    // raza de slowdown in jurul sau, fara damage direct
    return Tower("Honeypot", TowerType::HONEYPOT, 0.0f, 0.0f, 1.5f, 0.0f, 30, col, row);
}

Tower makeFirewall(int col, int row) {
    // plasat pe drum; absoarbe inamicii mai slabi decat HP-ul sau curent si se regenereaza in timp. 
    return Tower("Firewall", TowerType::FIREWALL, 0.0f, 0.0f, 0.0f, 0.0f, 60, col, row, 80.0f, 5.0f);
}



bool Tower::isInRange(const Enemy& enemy) const {
    float dx = enemy.getX() - static_cast<float>(x);
    float dy = enemy.getY() - static_cast<float>(y);
    return std::sqrt(dx * dx + dy * dy) <= range;
}

void Tower::attackEnemy(Enemy& enemy) {
    // calculeaza unde sa traga inainte de a trage (o sa fie posibil pe viitor ca turnurile sa rateze daca inamicii isi schimba viteza)
    std::pair<float, float> target = calculateInterceptPoint(enemy);
    float ix = target.first;
    float iy = target.second;
    std::cout << "  " << name << " fires -> predicted pos ("
              << static_cast<int>(ix) << "," << static_cast<int>(iy) << ")  "
              << enemy.getName() << " HP: " << static_cast<int>(enemy.getCurrentHealth())
              << " -> ";
    enemy.takeDamage(damage);
    std::cout << static_cast<int>(enemy.getCurrentHealth()) << "\n";
}

// Firewall: daca HP-ul inamicului <= HP-ul nostru, il absorb; altfel trece prin
void Tower::blockEnemy(Enemy& enemy) {
    if (enemy.getCurrentHealth() <= currentHP) {
        currentHP -= enemy.getCurrentHealth();
        enemy.takeDamage(enemy.getCurrentHealth()); // omoara inamicul
        std::cout << "  " << name << " absorbed " << enemy.getName()
                  << "! (Firewall HP: " << static_cast<int>(currentHP)
                  << "/" << static_cast<int>(maxHP) << ")\n";
    }
    // altfel: trece prin fara sa isi ia dmg
}

void Tower::applyHoneypotSlow(std::vector<Enemy>& enemies) {
    for (auto& enemy : enemies) {
        if (!enemy.isAlive()) continue;
        float dx   = enemy.getX() - static_cast<float>(x);
        float dy   = enemy.getY() - static_cast<float>(y);
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist <= range) {
            enemy.applySlow(0.8f); // reducere viteza cu 20%
        }
    }
}


// gaseste pozitia unde va fi inamicul  
std::pair<float, float> Tower::calculateInterceptPoint(const Enemy& enemy) const {
    float ex = enemy.getX();
    float ey = enemy.getY();
    float vx = enemy.getVelocityX();
    float vy = enemy.getVelocityY();
    float ps = projectileSpeed;

    float dx = ex - static_cast<float>(x);
    float dy = ey - static_cast<float>(y);

    // ecuatie de gr2
    float a = vx * vx + vy * vy - ps * ps;
    float b = 2.0f * (dx * vx + dy * vy);
    float c = dx * dx + dy * dy;

    float t = 0.0f;

    if (std::abs(a) < 0.0001f) {
        //proiectilul e mult mai rapid decat inamicul, sau inamicul sta pe loc
        t = (std::abs(b) > 0.0001f) ? (-c / b) : 0.0f;
    } else {
        float disc = b * b - 4.0f * a * c;
        if (disc < 0.0f) {
            return {ex, ey}; // nu exista solutie reala
        }
        float sqrtDisc = std::sqrt(disc);
        float t1 = (-b + sqrtDisc) / (2.0f * a);
        float t2 = (-b - sqrtDisc) / (2.0f * a);

        if      (t1 > 0.0f && t2 > 0.0f) t = std::min(t1, t2);
        else if (t1 > 0.0f)               t = t1;
        else if (t2 > 0.0f)               t = t2;
        else return {ex, ey}; // amandoua negative: nu putem intercepta
    }

    return {ex + vx * t, ey + vy * t};
}

// Update principal:cooldown-ul, alege o tinta, ataca sau aplica efect
void Tower::update(std::vector<Enemy>& enemies, float deltaTime) {
    if (type == TowerType::FIREWALL) {
        currentHP = std::min(currentHP + regenRate * deltaTime, maxHP); // regenerare HP
        blockEnemiesOnPath(enemies);
        return;
    }

    if (type == TowerType::HONEYPOT) {
        applyHoneypotSlow(enemies);
        return;
    }

    attackCooldown -= deltaTime;
    if (attackCooldown > 0.0f) return;

    // Gasim cel mai apropiat inamic viu din raza
    Enemy* target   = nullptr;
    float  minDist  = std::numeric_limits<float>::max();
    for (auto& enemy : enemies) {
        if (!enemy.isAlive() || !isInRange(enemy)) continue;
        float dx   = enemy.getX() - static_cast<float>(x);
        float dy   = enemy.getY() - static_cast<float>(y);
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist < minDist) {
            minDist = dist;
            target  = &enemy;
        }
    }

    if (target) {
        attackEnemy(*target);
        attackCooldown = 1.0f / attackSpeed;
    }
}

//Firewall 
void Tower::blockEnemiesOnPath(std::vector<Enemy>& enemies) {
    for (auto& enemy : enemies) {
        if (!enemy.isAlive()) continue;
        float dx   = enemy.getX() - static_cast<float>(x);
        float dy   = enemy.getY() - static_cast<float>(y);
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist < 0.6f) { // inamicul e pe celula firewall-ului
            blockEnemy(enemy);
        }
    }
}


TowerType          Tower::getType()  const { return type; }
int                Tower::getX()     const { return x; }
int                Tower::getY()     const { return y; }
int                Tower::getCost()  const { return cost; }
const std::string& Tower::getName()  const { return name; }

std::ostream& operator<<(std::ostream& os, const Tower& t) {
    os << "[" << t.name << " @(" << t.x << "," << t.y << ")";
    if (t.type == TowerType::FIREWALL) {
        os << " HP:" << static_cast<int>(t.currentHP) << "/" << static_cast<int>(t.maxHP);
    } else if (t.type == TowerType::HONEYPOT) {
        os << " slow:20% range:" << t.range;
    } else {
        os << " dmg:" << t.damage << " aspd:" << t.attackSpeed;
    }
    os << " cost:" << t.cost << "]";
    return os;
}
