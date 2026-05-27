#pragma once
#include <memory>
#include <random>
#include <vector>
#include <string>
#include "Evolution.h"
#include "EvolutionSpecs.h"
#include "EvolutionToken.h"
#include "MythicEvolution.h"
#include "WeightedTable.hpp"

// Factory Method pattern: o ierarhie de factory-uri, fiecare specializat pe un tier de
// evolutii (Mini / Major / Mythic). 
//
//toate spec-urile vin din data/evolutions.json, parsate o data la constructor.
class EvolutionFactory {
public:
    virtual ~EvolutionFactory() = default;

    // Numar de spec-uri incarcate.
    virtual int size() const = 0;

    // Nume tier (debug + UI label la categorie shop).
    virtual const char* tierName() const = 0;
};

//  MiniEvolutionFactory 
// Pool de MiniStatSpec uniform ponderate. sample() intoarce un spec aleator.
// La cumparare in shop, MiniStatSpec se aplica b pe GlobalStatBuffs (fara token).
class MiniEvolutionFactory : public EvolutionFactory {
    WeightedTable<MiniStatSpec> table_;

public:
    // Citeste "mini" din data/evolutions.json.
    explicit MiniEvolutionFactory(const std::string& path);

    int size() const override;
    const char* tierName() const override { return "Mini"; }

    const MiniStatSpec& sample(std::mt19937& rng) const;
};

// MajorEvolutionFactory
// Pool de MajorEvolutionSpec ponderate pe rarity (din "rarity_weights" in JSON).
// Spec poate fi stat (apply imediat) sau ability (genereaza EvolutionToken).
class MajorEvolutionFactory : public EvolutionFactory {
    WeightedTable<MajorEvolutionSpec> table_;

public:
    explicit MajorEvolutionFactory(const std::string& path);

    int size() const override;
    const char* tierName() const override { return "Major"; }

    const MajorEvolutionSpec& sample(std::mt19937& rng) const;
};

// MythicEvolutionFactory 
// se obtine doar prin combinarea a 2 token-uri Legendary care
// match-uiesc o reteta (mythic_recipes in JSON). not yet implemented
class MythicEvolutionFactory : public EvolutionFactory {
    std::vector<MythicRecipeSpec> recipes_;
    int mythic_cost_ = 0;

public:
    explicit MythicEvolutionFactory(const std::string& path);

    int size() const override;
    const char* tierName() const override { return "Mythic"; }

    // Cauta o reteta pentru perechea (a, b).
    bool canCraft(AbilityType a, AbilityType b, std::string* outName = nullptr) const;

    //Arunca IncompatibleEvolutionException daca:
    //oricare token nu e Legendary
    //combinatia ability nu match-uieste nicio reteta
    std::unique_ptr<MythicEvolution> craft(const EvolutionToken& a, const EvolutionToken& b) const;

    int mythicCost() const { return mythic_cost_; }
};
