#pragma once
#include <string>
#include <ostream>
#include <vector>
#include <utility>

struct EnemySpec;   // fwd

class Enemy {
    std::string name;
    float maxHealth;
    float currentHealth;
    float speed;
    int reward;
    int pathIndex;
    float x, y;
    float slowFactor;
    float vx, vy;

    float distanceTo(float targetX, float targetY) const;

public:
    // Constructor explicit (folosit momentan la testele T2 care creaza Enemy direct).
    Enemy(const std::string& name, float maxHealth, float speed, int reward);

    // T3 constructor din spec
    explicit Enemy(const EnemySpec& spec);

    void move(const std::vector<std::pair<int, int>>& path, float deltaTime);

    void takeDamage(float damage);
    void applySlow(float factor);
    void resetSlow();
    void placeAt(float startX, float startY);

    bool isAlive() const;
    bool hasReachedEnd(const std::vector<std::pair<int, int>>& path) const;

    float getX() const;
    float getY() const;
    float getCurrentHealth() const;
    float getMaxHealth() const;
    float getEffectiveSpeed() const;
    float getVelocityX() const;
    float getVelocityY() const;
    const std::string& getName() const;
    int getReward() const;

    friend std::ostream& operator<<(std::ostream& os, const Enemy& e);
};
