#pragma once

// Enum global pentru ability types. Mutat din AbilityEvolution pentru a sparge
// Tower cu AbilityEvolution (Tower base trebuie sa stie AbilityType
// pentru virtual applyAbility(AbilityType), dar nu ar trebui sa includa Evolution.h).
//
// AbilityEvolution.h pastreaza numele AbilityEvolution::AbilityType pentru
// compatibilitate cu cod existent.
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
