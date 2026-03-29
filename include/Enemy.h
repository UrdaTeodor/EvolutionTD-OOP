#pragma once
#include <string>
#include <ostream>
#include <vector>
#include <utility>

class Enemy {
    std::string name;
    float maxHealth;
    float currentHealth;
    float speed;       
    int reward;        // credits pe kill
    int pathIndex;     // index pentru urmatorul waypoint din path
    float x, y;        // current position
    float slowFactor; 
    float vx, vy;      // current velocity

    float distanceTo(float targetX, float targetY) const;

public:
    Enemy(const std::string& name, float maxHealth, float speed, int reward);

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
    float getEffectiveSpeed() const; // speed * slowFactor
    float getVelocityX() const;
    float getVelocityY() const;
    const std::string& getName() const;
    int getReward() const;

    friend std::ostream& operator<<(std::ostream& os, const Enemy& e);
};


Enemy makeAdware();
Enemy makeTrojan();
Enemy makeWorm();
