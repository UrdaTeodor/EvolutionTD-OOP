#pragma once
#include "Evolution.h"
#include "AbilityEvolution.h"
#include "MythicType.h"

// MythicEvolution: combinatie de 2 AbilityEvolution Legendary.
// apply() aplica ambele abilitati-sursa + efectul unic (Tower::applyMythic).
class MythicEvolution : public Evolution {
public:
    // Alias pastrat pentru codul existent; enum-ul real e in MythicType.h
    // (partajat cu Tower::applyMythic).
    using MythicType = ::MythicType;

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
