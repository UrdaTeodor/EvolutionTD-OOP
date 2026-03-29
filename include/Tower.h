#pragma once
#include <string>
#include <ostream>
#include <vector>
#include <utility>
#include "Enemy.h"

enum class TowerType { ANTIVIRUS, ADBLOCKER, HONEYPOT, FIREWALL };

class Tower {
    std::string name;
    TowerType type;
    float damage;
    float attackSpeed;      
    float range;            
    float projectileSpeed;  
    int cost;
    int x, y;               // position
    float attackCooldown;   
    float currentHP;        // Firewall 
    float maxHP;            // Firewall 
    float regenRate;        // Firewall 

    bool isInRange(const Enemy& enemy) const;
    void attackEnemy(Enemy& enemy);
    void blockEnemy(Enemy& enemy); // Firewall


    void applyHoneypotSlow(std::vector<Enemy>& enemies);

public:
    Tower(const std::string& name, TowerType type,
          float damage, float attackSpeed, float range,
          float projectileSpeed, int cost, int x, int y,
          float maxHP = 0.0f, float regenRate = 0.0f);


    std::pair<float, float> calculateInterceptPoint(const Enemy& enemy) const;


    void update(std::vector<Enemy>& enemies, float deltaTime);


    void blockEnemiesOnPath(std::vector<Enemy>& enemies);

    TowerType getType() const;
    int getX() const;
    int getY() const;
    int getCost() const;
    const std::string& getName() const;

    friend std::ostream& operator<<(std::ostream& os, const Tower& t);
};


Tower makeAntivirus(int col, int row);
Tower makeAdblocker(int col, int row);
Tower makeHoneypot(int col, int row);
Tower makeFirewall(int col, int row);
