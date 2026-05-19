#pragma once
#include "Evolution.h"

// Folosit pentru Epic si Legendary Major.
// Tipul abilitatii e ales prin enum-ul AbilityType.
class AbilityEvolution : public Evolution {
public:
    enum class AbilityType {
        // ---- Epic ----
        MULTI_TARGET,         // dubleaza numarul de atacuri simultane 
                              //(ex. 2 proiectile in loc de 1 dar pe inamici diferiti,
                              // nu schimba cu nimic daca exista un singur inamic in raza)
        BIGGER_AURA,          // Honeypot: raza permanent mai mare
        ARMORED,              // flat damage reduction (da reward sa tankezi inamici multi si slabi)

        // ---- Legendary (3 combinatii valide pentru Mythic) ----
        DOUBLE_SHOT,          // atac: 2 proiectile la 80% dmg fiecare
        FIRE_TRAIL,           // atac: proiectila lasa urma de foc (20% dmg pe secunda timp de 3 sec)
        KNOCKBACK_EVERY_3,    // atac: la fiecare al 3-lea shot, knockback
        REFLECTIVE_SHIELD,    // redirectioneaza inamicul inapoi pe track si scade hp ramas din hp urmatorului inamic (cooldown 5s)(merge maxim 3 patratele inapoi)
        MOVABLE               // orice turn: poate fi mutat o data per val
    };

private:
    AbilityType ability;

public:
    AbilityEvolution(std::string name, int cost, Rarity rarity, AbilityType ability);

    void apply(Tower& target) override;
    std::unique_ptr<Evolution> clone() const override;

    AbilityType getAbility() const;

    // STATIC: verifica daca 2 abilitati Legendary se pot combina in Mythic.
    // Hardcodat: doar 3 perechi (din 10 posibile) sunt valide.
    static bool canCombine(AbilityType a, AbilityType b);

protected:
    void displayDetails(std::ostream& os) const override;
};
