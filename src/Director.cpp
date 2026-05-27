#include "Director.h"
#include <algorithm>
#include <climits>

Director::Director(std::mt19937& rng)
    : rng_(&rng) {}

// cppcheck-suppress unusedFunction
void Director::unlock(const std::string& key, const EnemySpec& spec, float weight) {
    // evita duplicate (idempotent la mai multe unlock-uri pentru aceeasi cheie)
    if (std::find(unlocked_keys_.begin(), unlocked_keys_.end(), key) != unlocked_keys_.end()) {
        return;
    }
    pool_.add(spec, weight);
    unlocked_keys_.push_back(key);
}

// cppcheck-suppress unusedFunction
std::vector<EnemySpec> Director::generateSpawns(int budget) const {
    std::vector<EnemySpec> out;
    if (pool_.empty() || budget <= 0) return out;

    // safety: daca toti inamicii din pool au cost > budget, nu cyclam la infinit.
    int min_cost = INT_MAX;
    for (const auto& spec : pool_.items()) {
        if (spec.director_cost > 0 && spec.director_cost < min_cost) {
            min_cost = spec.director_cost;
        }
    }
    if (min_cost == INT_MAX || min_cost > budget) return out;

    // Greedy spend. Safety limit pentru loop.
    for (int iter = 0; iter < 200 && budget >= min_cost; ++iter) {
        const EnemySpec& pick = pool_.sample(*rng_);
        if (pick.director_cost > 0 && pick.director_cost <= budget) {
            out.push_back(pick);
            budget -= pick.director_cost;
        }
        // pick.director_cost == 0 => skip, sample altul
        // pick > budget => sample altul, eventual loop iese cand budget < min_cost
    }
    return out;
}
