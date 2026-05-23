#pragma once
#include <vector>
#include <random>
#include <stdexcept>
#include <utility>
#include <cstddef>

// Class template (cerinta T3, min 2 instantieri):
//   WeightedTable<EnemySpec>          (Director inamici) 
//   WeightedTable<MiniStatSpec>        Shop Mini drop 
//   WeightedTable<MajorEvolutionSpec>  Shop Major drop 
//
// Memoreaza pereche (item, weight). sample(rng) intoarce un item ales in functie de ponderi.
// roll uniform in [0, total_weight], scan cumulativ. O(N) sample.
// La N=3-10 
template <typename T>
class WeightedTable {
    std::vector<T>     items_;
    std::vector<float> weights_;
    float              total_weight_ = 0.0f;

public:
    WeightedTable() = default;

    // Universal reference permite atat T copy-able cat si move-only (ex. unique_ptr).
    template <typename U>
    void add(U&& item, float weight) {
        if (weight < 0.0f) {
            throw std::invalid_argument("WeightedTable.add: weight negativ");
        }
        items_.push_back(std::forward<U>(item));
        weights_.push_back(weight);
        total_weight_ += weight;
    }

    bool   empty() const { return items_.empty(); }
    std::size_t size()  const { return items_.size(); }
    float  totalWeight() const { return total_weight_; }

    // Sample weighted. Arunca daca tabela e goala.
    // Returneaza const ref ca sa evitam copii inutile la T mari
    const T& sample(std::mt19937& rng) const {
        if (items_.empty()) {
            throw std::runtime_error("WeightedTable.sample: tabela goala");
        }
        if (total_weight_ <= 0.0f) {
            // toti weights = 0  uniform fallback
            std::uniform_int_distribution<std::size_t> uni(0, items_.size() - 1);
            return items_[uni(rng)];
        }
        std::uniform_real_distribution<float> dist(0.0f, total_weight_);
        float roll = dist(rng);
        float cumulative = 0.0f;
        for (std::size_t i = 0; i < items_.size(); ++i) {
            cumulative += weights_[i];
            if (roll <= cumulative) return items_[i];
        }
        return items_.back();   //edge case roll == total_weight
    }

    //rendering UI (afisare carduri) si save.
    const std::vector<T>&     items()   const { return items_; }
    const std::vector<float>& weights() const { return weights_; }
};
