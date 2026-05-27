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
    // Wave::simulate apeleaza tickFireTrail per frame ca sa scada HP.
    float fire_trail_dps_       = 0.0f;   // dps
    float fire_trail_remaining_ = 0.0f;   // sec ramase

    float distanceTo(float targetX, float targetY) const;

public:
    // Constructor explicit t2
    Enemy(const std::string& name, float maxHealth, float speed, int reward);

    // T3 constructor din spec
    explicit Enemy(const EnemySpec& spec);

    void move(const std::vector<std::pair<int, int>>& path, float deltaTime);

    void takeDamage(float damage);
    void applySlow(float factor);
    void resetSlow();
    void placeAt(float startX, float startY);
    // Director scaling multiplica maxHealth + currentHealth la create.
    void scaleHealth(float multiplier);

    // FIRE_TRAIL aplica DoT pe enemy. Daca exista deja un trail, refresh la noua valoare
    //  Apelat de Antivirus/Adblocker dupa attack daca au fireTrail flag.
    void applyFireTrail(float dps, float duration_sec);
    // Wave apeleaza per-frame. Scade HP cu dps*dt si decrementeaza remaining.
    void tickFireTrail(float dt);

    // KNOCKBACK / REFLECTIVE_SHIELD muta enemy inapoi pe path cu N cells.
    // Clamp pathIndex la 1 minim (nu poate iesi din spawn).
    void pushBack(int cells, const std::vector<std::pair<int, int>>& path);

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
