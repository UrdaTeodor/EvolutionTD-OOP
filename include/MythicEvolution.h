#pragma once
#include "Evolution.h"
#include "AbilityEvolution.h"

// MythicEvolution: combinatie de 2 AbilityEvolution Legendary.
class MythicEvolution : public Evolution {
public:
    enum class MythicType {
        PHOENIX_BARRAGE,    // DOUBLE_SHOT + FIRE_TRAIL placeholdere momentan
        ROVING_BRUISER,     // KNOCKBACK_EVERY_3 + MOVABLE
        SHIELDED_RUNNER     // REFLECTIVE_SHIELD + MOVABLE
    };

private:
    std::unique_ptr<AbilityEvolution> source1;
    std::unique_ptr<AbilityEvolution> source2;
    MythicType mythicType;

    // helper  deduce ce tip de Mythic rezulta din 2 abilitati legendare
    static MythicType deriveType(AbilityEvolution::AbilityType a,
                                 AbilityEvolution::AbilityType b);

public:
    // Constructor preia 2 surse (consumate).
    MythicEvolution(std::string name, int cost,
                    std::unique_ptr<AbilityEvolution> a,
                    std::unique_ptr<AbilityEvolution> b);

    // Regula celor 5 (pentru ca detinem unique_ptr)
    MythicEvolution(const MythicEvolution& other);
    MythicEvolution& operator=(MythicEvolution other);   // copy-and-swap (other = by value)
    MythicEvolution(MythicEvolution&&) noexcept = default;
    MythicEvolution& operator=(MythicEvolution&&) noexcept = default;
    ~MythicEvolution() override = default;

    void apply(const EvolutionContext& ctx) override;
    std::unique_ptr<Evolution> clone() const override;

    friend void swap(MythicEvolution& a, MythicEvolution& b) noexcept;

protected:
    void displayDetails(std::ostream& os) const override;
};
