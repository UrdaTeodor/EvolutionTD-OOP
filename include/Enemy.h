#pragma once
#include <string>
#include <ostream>
#include <vector>
#include <utility>

// Diferenta intre Adware/Trojan/Worm e doar in NUMERE (HP, viteza, reward),
class Enemy {
    // toate atributele private = doar Enemy poate sa le modifice direct;
    // restul codului trece prin metode publice (getteri, takeDamage, etc.)
    std::string name;
    float maxHealth;
    float currentHealth;
    float speed;
    int reward;        // credits pe kill
    int pathIndex;     // index pentru urmatorul waypoint din path
    float x, y;        // pozitie curenta
    float slowFactor;  // 1.0 = viteza normala, 0.8 = 20% mai lent
    float vx, vy;      // viteza curenta

    float distanceTo(float targetX, float targetY) const;

public:
    // constructor de initializare
    Enemy(const std::string& name, float maxHealth, float speed, int reward);

    // muta inamicul pe path
    // Apelata din Wave::simulate (src/Wave.cpp).
    void move(const std::vector<std::pair<int, int>>& path, float deltaTime);

    void takeDamage(float damage);    // scade currentHealth
    void applySlow(float factor);     // seteaza slowFactor (apelata din Honeypot)
    void resetSlow();                 // slowFactor = 1.0
    void placeAt(float startX, float startY);  // muta inamicul la startul path-ului

    bool isAlive() const;
    bool hasReachedEnd(const std::vector<std::pair<int, int>>& path) const;


    float getX() const;
    float getY() const;
    float getCurrentHealth() const;
    float getEffectiveSpeed() const;  // speed * slowFactor
    float getVelocityX() const;
    float getVelocityY() const;
    const std::string& getName() const;
    int getReward() const;

    // friend: operator<< poate accesa atributele private ale Enemy
    friend std::ostream& operator<<(std::ostream& os, const Enemy& e);
};

// factory free functions: construiesc Enemy cu stats hardcodate.
Enemy makeAdware();
Enemy makeTrojan();
Enemy makeWorm();
