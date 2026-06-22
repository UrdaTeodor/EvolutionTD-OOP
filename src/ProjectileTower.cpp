#include "ProjectileTower.h"
#include "GlobalStatBuffs.h"
#include "GameException.h"
#include <algorithm>
#include <cmath>
//disclaimer: Mult AI
ProjectileTower::ProjectileTower(const TowerSpec& spec, std::string type_key,
                                 int col, int row)
    : Tower(spec, std::move(type_key), col, row) {}

bool ProjectileTower::isInRange(const Enemy& enemy, float effectiveRange) const {
    float dx = enemy.getX() - static_cast<float>(getX());
    float dy = enemy.getY() - static_cast<float>(getY());
    return std::sqrt(dx * dx + dy * dy) <= effectiveRange;
}

float ProjectileTower::targetScore(const Enemy& enemy,
                                   const std::vector<std::pair<int, int>>& path) const {
    float dx = enemy.getX() - static_cast<float>(getX());
    float dy = enemy.getY() - static_cast<float>(getY());
    switch (targeting()) {
        case TargetingMode::CLOSE:
            return -std::sqrt(dx * dx + dy * dy);
        case TargetingMode::STRONG:
            // HP-ul domina; progresul pe drum departajeaza intre HP-uri egale.
            return enemy.getCurrentHealth() * 10000.0f + enemy.pathProgress(path);
        case TargetingMode::LAST:
            // Cel mai din spate: FireTrail are timpul maxim sa arda inainte
            // ca inamicul sa se apropie de baza.
            return -enemy.pathProgress(path);
        case TargetingMode::FIRST:
        default:
            return enemy.pathProgress(path);
    }
}

std::vector<size_t> ProjectileTower::targetIndices(
        const std::vector<Enemy>& enemies, float range,
        const std::vector<std::pair<int, int>>& path, int maxCount) const {
    std::vector<std::pair<float, size_t>> scored;
    for (size_t i = 0; i < enemies.size(); ++i) {
        if (!enemies[i].isAlive() || !isInRange(enemies[i], range)) continue;
        // Predicted death: nu trage in tinte pe care proiectilele din zbor
        // le vor ucide oricum (focul ar disparea ca fizzle).
        if (enemies[i].isPredictedDead()) continue;
        scored.push_back({ targetScore(enemies[i], path), i });
    }
    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });
    if (static_cast<int>(scored.size()) > maxCount) scored.resize(maxCount);

    std::vector<size_t> out;
    out.reserve(scored.size());
    for (const auto& [score, idx] : scored) out.push_back(idx);
    return out;
}

int ProjectileTower::shotStyleMask() const {
    int mask = 0;
    if (phoenix_) {
        mask |= shotstyle::PHOENIX;          // ascunde DoubleShot+FireTrail (ingrediente)
    } else {
        if (doubleShotStacks > 0) mask |= shotstyle::DOUBLE_SHOT;
        if (fireTrailStacks  > 0) mask |= shotstyle::FIRE_TRAIL;
    }
    if (shockwave_) {
        mask |= shotstyle::BRUISER;          // ascunde Knockback (ingredient)
    } else if (knockbackInterval > 0) {
        mask |= shotstyle::KNOCKBACK;
    }
    if (multiTargetStacks > 0) mask |= shotstyle::EPIC;
    if (splashStacks      > 0) mask |= shotstyle::SPLASH;
    return mask;
}

void ProjectileTower::update(std::vector<Enemy>& enemies, float deltaTime,
                             const GlobalStatBuffs& buffs,
                             const std::vector<std::pair<int, int>>& path,
                             std::vector<Shot>& out_shots) {
    attackCooldown -= deltaTime;

    knock_window_ -= deltaTime;
    if (knock_window_ <= 0.0f) {
        knock_window_ = 1.0f;
        knock_procs_  = 0;
    }

    if (phoenix_ && embers_ >= 10) {
        embers_ -= 10;
        float dps = spec().damage * (1.0f + buffs.pct(getTypeKey(), "damage_pct"))
                  * 0.3f * std::max(1, fireTrailStacks);
        for (auto& e : enemies) {
            if (e.isAlive()) e.applyFireTrail(dps, 4.0f);
        }
    }

    if (attackCooldown > 0.0f) return;


    float damage_effective       = spec().damage
                                 * (1.0f + buffs.pct(getTypeKey(), "damage_pct"))
                                 * (1.0f + localDamagePct());
    float attack_speed_effective = spec().attack_speed
                                 * (1.0f + buffs.pct(getTypeKey(), "attack_speed_pct"))
                                 * (1.0f + localAttackSpeedPct());
    float range                  = effectiveRange(buffs);

    // MULTI_TARGET -> top (1 + stacks) tinte dupa prioritatea de targeting.
    int max_targets = 1 + multiTargetStacks;
    std::vector<Enemy*> targets;
    for (size_t idx : targetIndices(enemies, range, path, max_targets)) {
        targets.push_back(&enemies[idx]);
    }
    if (targets.empty()) return;

    int   attacks_per_target = 1 + doubleShotStacks;

    constexpr float EXTRA_SHOT_SCALE   = 0.60f;
    constexpr float EXTRA_TARGET_SCALE = 1.00f;

    const int   style    = shotStyleMask();
    const float fire_dps = (fireTrailStacks > 0)
                               ? damage_effective * 0.9f * fireTrailStacks
                               : 0.0f;

    auto makeShot = [&](const Enemy& target, float dmg, float delay) {
        Shot s;
        s.target_id       = target.id();
        s.x               = static_cast<float>(getX());
        s.y               = static_cast<float>(getY());
        s.speed           = (spec().projectile_speed > 0.0f) ? spec().projectile_speed : 8.0f;
        s.damage          = dmg;
        s.fire_dps        = fire_dps;
        s.delay           = delay;
        s.source_char     = getDisplayChar();
        s.source_tower_id = instanceId();
        s.style_mask      = style;
        s.pierce_left     = doubleShotStacks;          // pierce clasic per stack
        s.splash_pct      = 0.35f * static_cast<float>(splashStacks);
        return s;
    };

    for (size_t ti = 0; ti < targets.size(); ++ti) {
        Enemy* target = targets[ti];
        // EXTRA_TARGET_SCALE e 1.0 by design acum (tinte secundare la 100%);
        // ternarul ramane ca buton de reglaj => ambele ramuri egale e intentionat.
        // cppcheck-suppress duplicateExpressionTernary
        float damage_per_attack = damage_effective * (ti == 0 ? 1.0f : EXTRA_TARGET_SCALE);

        // Evantai MultiTarget: focurile spre tinte diferite pleaca decalat cu
        // 40ms, in ordine — rafala se CITESTE ("matura valul"), nu se suprapune.
        float fan_delay = 0.04f * static_cast<float>(ti);

        for (int a = 0; a < attacks_per_target; ++a) {
            float dmg = damage_per_attack * (a == 0 ? 1.0f : EXTRA_SHOT_SCALE);
            Shot s = makeShot(*target, dmg, fan_delay + 0.09f * static_cast<float>(a));

            // Knockback marcat la TRAGERE (a N-a lovitura), aplicat la impact.
            ++shotCounter;
            if (knockbackInterval > 0 && shotCounter % knockbackInterval == 0) {
                if (knock_procs_ < KNOCK_CAP_PER_SEC) {
                    s.knockback   = true;
                    s.knock_cells = knockbackCells;
                    s.shockwave   = shockwave_;
                    ++knock_procs_;
                } else {
                    s.excess_slow = true;   // peste cap: slow 50% / 1s
                }
            }
            out_shots.push_back(s);
            // Anunta si turnurile care trag DUPA noi in acelasi tick.
            target->addIncomingDamage(dmg);
        }
    }

    attackCooldown = 1.0f / attack_speed_effective;
}


void ProjectileTower::applyAbility(AbilityType a) {
    switch (a) {
        case AbilityType::DOUBLE_SHOT:  ++doubleShotStacks;  break;
        case AbilityType::FIRE_TRAIL:   ++fireTrailStacks;   break;
        case AbilityType::MULTI_TARGET: ++multiTargetStacks; break;
        case AbilityType::SPLASH:       ++splashStacks;      break;
        case AbilityType::MOVABLE:      enableMovable();     break;
        default:
            Tower::applyAbility(a);
    }
}

void ProjectileTower::setKnockbackInterval(int N) {
    // Al doilea token de Knockback nu ar schimba nimic (intervalul e fix 3).
    if (knockbackInterval > 0) {
        throw IncompatibleEvolutionException(
            "Knockback nu se stackeaza (turnul il are deja).");
    }
    knockbackInterval = N;
}

void ProjectileTower::applyMythic(MythicType m) {
    if (mythicBadge() != nullptr) {
        throw IncompatibleEvolutionException(
            "Turnul are deja un Mythic (max 1 per turn).");
    }
    switch (m) {
        case MythicType::PHOENIX_BARRAGE:
            phoenix_ = true;
            break;
        case MythicType::ROVING_BRUISER:
            shockwave_        = true;
            knockbackInterval = 2;   // la fiecare a 2-a lovitura (in loc de a 3-a)
            knockbackCells    = 2;   // arunca 2 celule (in loc de 1)
            break;
        default:
            Tower::applyMythic(m);   // throw incompatibil
    }
}

const char* ProjectileTower::mythicBadge() const {
    if (phoenix_)   return "PhoenixBarrage";
    if (shockwave_) return "RovingBruiser";
    return nullptr;
}

std::vector<AbilityType> ProjectileTower::getAppliedAbilities() const {
    auto result = Tower::getAppliedAbilities();
    for (int i = 0; i < multiTargetStacks; ++i) result.push_back(AbilityType::MULTI_TARGET);
    for (int i = 0; i < doubleShotStacks; ++i)  result.push_back(AbilityType::DOUBLE_SHOT);
    for (int i = 0; i < fireTrailStacks; ++i)   result.push_back(AbilityType::FIRE_TRAIL);
    for (int i = 0; i < splashStacks; ++i)      result.push_back(AbilityType::SPLASH);
    if (knockbackInterval > 0) result.push_back(AbilityType::KNOCKBACK_EVERY_3);
    return result;
}

void ProjectileTower::displayDetails(std::ostream& os) const {
    os << " dmg:" << spec().damage << " aspd:" << spec().attack_speed;
    if (doubleShotStacks  > 0) os << " [DoubleShot x"  << doubleShotStacks  << "]";
    if (fireTrailStacks   > 0) os << " [FireTrail x"   << fireTrailStacks   << "]";
    if (multiTargetStacks > 0) os << " [MultiTarget x" << multiTargetStacks << "]";
    if (splashStacks      > 0) os << " [BlastWave x"   << splashStacks      << "]";
    if (knockbackInterval > 0) os << " [Knockback/" << knockbackInterval << "]";
}
