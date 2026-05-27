#include "EvolutionFactory.h"
#include "AbilityEvolution.h"
#include "GameException.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>

namespace {
    nlohmann::json loadJson(const std::string& path) {
        std::ifstream in(path);
        if (!in) {
            throw DataException("Nu pot deschide fisierul JSON: " + path);
        }
        nlohmann::json j;
        try {
            in >> j;
        } catch (const nlohmann::json::parse_error& err) {
            throw DataException("Parse error in " + path + ": " + err.what());
        }
        return j;
    }
}

// =====================================================================
// MiniEvolutionFactory
// =====================================================================

MiniEvolutionFactory::MiniEvolutionFactory(const std::string& path) {
    nlohmann::json j = loadJson(path);
    if (!j.contains("mini")) {
        throw DataException(path + ": lipseste cheia 'mini'");
    }
    for (const auto& mini_j : j.at("mini")) {
        MiniStatSpec spec = mini_j.get<MiniStatSpec>();
        // Ponderi uniforme pentru Mini (toate la fel de probabile la sample).
        table_.add(std::move(spec), 1.0f);
    }
    if (table_.empty()) {
        throw DataException(path + ": pool 'mini' gol");
    }
}

int MiniEvolutionFactory::size() const {
    return static_cast<int>(table_.size());
}

const MiniStatSpec& MiniEvolutionFactory::sample(std::mt19937& rng) const {
    return table_.sample(rng);
}

// =====================================================================
// MajorEvolutionFactory
// =====================================================================

MajorEvolutionFactory::MajorEvolutionFactory(const std::string& path) {
    nlohmann::json j = loadJson(path);
    if (!j.contains("major") || !j.contains("rarity_weights")) {
        throw DataException(path + ": lipsesc cheile 'major' / 'rarity_weights'");
    }
    const auto& rar_weights = j.at("rarity_weights");

    for (const auto& major_j : j.at("major")) {
        MajorEvolutionSpec spec = major_j.get<MajorEvolutionSpec>();
        // Pondere pe rarity (Rare > Epic > Legendary in mod normal).
        std::string rar_str;
        major_j.at("rarity").get_to(rar_str);
        float weight = rar_weights.value(rar_str, 1.0f);
        table_.add(std::move(spec), weight);
    }
    if (table_.empty()) {
        throw DataException(path + ": pool 'major' gol");
    }
}

int MajorEvolutionFactory::size() const {
    return static_cast<int>(table_.size());
}

const MajorEvolutionSpec& MajorEvolutionFactory::sample(std::mt19937& rng) const {
    return table_.sample(rng);
}


// MythicEvolutionFactory


MythicEvolutionFactory::MythicEvolutionFactory(const std::string& path) {
    nlohmann::json j = loadJson(path);
    if (!j.contains("mythic_recipes")) {
        throw DataException(path + ": lipseste cheia 'mythic_recipes'");
    }
    for (const auto& recipe_j : j.at("mythic_recipes")) {
        recipes_.push_back(recipe_j.get<MythicRecipeSpec>());
    }
    mythic_cost_ = j.value("mythic_cost", 500);

    if (recipes_.empty()) {
        throw DataException(path + ": niciun mythic_recipe definit");
    }
}

int MythicEvolutionFactory::size() const {
    return static_cast<int>(recipes_.size());
}

bool MythicEvolutionFactory::canCraft(AbilityType a, AbilityType b,
                                      std::string* outName) const {
    for (const auto& r : recipes_) {
        bool match = (r.ingredient_a == a && r.ingredient_b == b) ||
                     (r.ingredient_a == b && r.ingredient_b == a);
        if (match) {
            if (outName) *outName = r.result_name;
            return true;
        }
    }
    return false;
}

std::unique_ptr<MythicEvolution> MythicEvolutionFactory::craft(
        const EvolutionToken& a, const EvolutionToken& b) const {
    if (a.rarity != Evolution::Rarity::LEGENDARY ||
        b.rarity != Evolution::Rarity::LEGENDARY) {
        throw IncompatibleEvolutionException(
            "MythicEvolutionFactory.craft: ambele token-uri trebuie sa fie Legendary");
    }

    std::string result_name;
    if (!canCraft(a.ability, b.ability, &result_name)) {
        throw IncompatibleEvolutionException(
            "MythicEvolutionFactory.craft: combinatia de abilitati nu match-uieste nicio reteta");
    }

    // Reconstruim cele 2 AbilityEvolution surse pe care MythicEvolution le detine.
    auto src_a = std::make_unique<AbilityEvolution>(
        a.name, 500, Evolution::Rarity::LEGENDARY, a.ability);
    auto src_b = std::make_unique<AbilityEvolution>(
        b.name, 500, Evolution::Rarity::LEGENDARY, b.ability);

    return std::make_unique<MythicEvolution>(
        result_name, mythic_cost_, std::move(src_a), std::move(src_b));
}
