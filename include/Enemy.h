#pragma once
#include <string>
#include <ostream>
#include <vector>
#include <utility>

struct EnemySpec;   // fwd

class Enemy {
    // Id unic per inamic spawnat, pastrat la copiere. Inamicii sunt COPIATI
    // intre vectori la fiecare frame (pending -> active -> survivors), deci
    // pointerii/indecsii nu sunt stabili; id-ul e singura cheie sigura pentru
    // tracking intre frame-uri (folosit de EffectsLayer pt damage numbers).
    static int next_id_;
    int id_;

    std::string name;
    float maxHealth;
    float currentHealth;
    float speed;
    int reward;
    int leakDamage = 5;     // HP pierdut de jucator daca inamicul ajunge la baza
    bool isBoss_   = false; // bosii sunt imuni la knockback (anti stall-cheese)
    int pathIndex;
    float x, y;
    float slowFactor;
    float vx, vy;
    // Wave::simulate apeleaza tickFireTrail per frame ca sa scada HP.
    float fire_trail_dps_       = 0.0f;   // dps
    float fire_trail_remaining_ = 0.0f;   // sec ramase

    // Statusuri Mythic (v0.4). Anti-stack: ambele se REIMPROSPATEAZA la max,
    // nu se aduna (acelasi pattern ca applyFireTrail).
    float stun_remaining_       = 0.0f;   // ShieldedRunner: inamicul sta pe loc
    float vulnerable_remaining_ = 0.0f;   // RovingBruiser: +25% damage primit

    // Knockback FIZIC: inamicul aluneca vizibil inapoi pe path (~0.3s), nu se
    // teleporteaza. kb_dist_left_ = celule ramase de parcurs inapoi.
    float kb_dist_left_ = 0.0f;
    float kb_speed_     = 0.0f;

    // Slow temporizat (anti-cheese knockback): separat de slow-ul de Honeypot
    // (care e resetat si reaplicat in fiecare frame).
    float tslow_remaining_ = 0.0f;
    float tslow_factor_    = 1.0f;

    // Damage "in zbor" spre inamicul asta (suma proiectilelor care il urmaresc).
    // Recalculat de Wave la fiecare tick; turnurile NU mai tintesc inamicii cu
    // incoming >= HP (predicted death) — altfel fiecare kill irosea ~un foc.
    float incoming_damage_ = 0.0f;

    void moveBackward(float step, const std::vector<std::pair<int, int>>& path);

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

    // Statusuri Mythic. Refresh la max, nu stack.
    void applyStun(float seconds);
    void applyVulnerability(float seconds);
    // Slow temporizat (excesul de knockback peste cap-ul pe secunda).
    void applyTimedSlow(float factor, float seconds);

    // Predicted death (vezi incoming_damage_).
    void  setIncomingDamage(float v)  { incoming_damage_ = v; }
    void  addIncomingDamage(float v)  { incoming_damage_ += v; }
    float incomingDamage() const      { return incoming_damage_; }
    bool  isPredictedDead() const     { return incoming_damage_ >= currentHealth; }
    bool isStunned()    const { return stun_remaining_ > 0.0f; }
    bool isVulnerable() const { return vulnerable_remaining_ > 0.0f; }
    bool isBurning()    const { return fire_trail_remaining_ > 0.0f; }
    // Orice forma de incetinire (Honeypot sau slow temporizat) — pentru
    // tenta albastra din BoardRenderer.
    bool isSlowed()     const { return slowFactor < 1.0f || tslow_remaining_ > 0.0f; }

    bool isAlive() const;
    bool hasReachedEnd(const std::vector<std::pair<int, int>>& path) const;

    int id() const { return id_; }

    // Cat de avansat e inamicul pe drum (mai mare = mai aproape de baza).
    // Folosit de targeting-ul FIRST. Nu e o distanta exacta in celule, ci un
    // scor monoton: waypoint-ul curent domina, distanta pana la el departajeaza.
    float pathProgress(const std::vector<std::pair<int, int>>& path) const;

    float getX() const;
    float getY() const;
    float getCurrentHealth() const;
    float getMaxHealth() const;
    float getEffectiveSpeed() const;
    float getVelocityX() const;
    float getVelocityY() const;
    const std::string& getName() const;
    int getReward() const;
    int getLeakDamage() const;

    friend std::ostream& operator<<(std::ostream& os, const Enemy& e);
};
