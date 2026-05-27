// EvolutionTD - Digital Immune System
// Z3: main.cpp e doar bootstrap. Toata logica de scene traieste in
//   - MainMenuScene (Start / Quit)
//   - GameScene     (joc activ, FF, undo, place tower)
//
// Tranzitiile sunt facute prin SceneManager (Strategy pattern).
#include <SFML/Graphics.hpp>
#include <iostream>
#include <memory>
#include "DataRegistry.h"
#include "GameException.h"
#include "Wave.h"
#include "Enemy.h"
#include "AntivirusTower.h"
#include "AbilityEvolution.h"
#include "EvolutionFactory.h"
#include "EvolutionContext.h"
#include "EvolutionToken.h"
#include "GlobalStatBuffs.h"
#include "SceneManager.h"
#include "MainMenuScene.h"

namespace {

// Smoke test T2 (ramane in consola, ruleaza inainte de fereastra).
void runT2DemoTests(const DataRegistry& registry) {
    // Regula celor 3 pentru Wave (cc + op= + dtor implicit corect)
    {
        Wave w1(0, {});
        w1.addEnemy(Enemy(registry.getEnemy("adware")));

        Wave w2 = w1;
        w2.addEnemy(Enemy(registry.getEnemy("trojan")));

        Wave w3(0, {});
        w3 = w1;
        w3.addEnemy(Enemy(registry.getEnemy("worm")));

        std::cout << "=== Test Regula celor 3 ===\n";
        std::cout << "w1 original (1 inamic):         " << w1;
        std::cout << "w2 copie cc (2 inamici):        " << w2;
        std::cout << "w3 copie op= (2 inamici):       " << w3;
        std::cout << "=> w1 are tot 1 inamic: copiile sunt independente\n";
    }

    // Ierarhia proprie de exceptii (3+1 derivate prinse prin baza GameException)
    std::cout << "=== Test excepții (GameException + 4 derivate) ===\n";

    try {
        MythicEvolutionFactory mythicFactory("data/evolutions.json");
        EvolutionToken a{ "LegendaryDoubleShot",
                          AbilityType::DOUBLE_SHOT,       Evolution::Rarity::LEGENDARY };
        EvolutionToken b{ "LegendaryReflectShield",
                          AbilityType::REFLECTIVE_SHIELD, Evolution::Rarity::LEGENDARY };
        // DOUBLE_SHOT + REFLECTIVE_SHIELD nu e in mythic_recipes -> arunca.
        mythicFactory.craft(a, b);
        std::cout << "  [BUG] throw\n";
    } catch (const IncompatibleEvolutionException& err) {
        std::cout << "  [ok] prins IncompatibleEvolutionException: " << err.what() << "\n";
    }

    try {
        AntivirusTower anti(registry.getTower("antivirus"), 0, 0);
        AbilityEvolution wrong("EpicBiggerAura", 100, Evolution::Rarity::EPIC,
                               AbilityEvolution::AbilityType::BIGGER_AURA);
        GlobalStatBuffs dummyBuffs;
        EvolutionContext ctx{&dummyBuffs, "antivirus", &anti};
        wrong.apply(ctx);
        std::cout << "  [BUG] throw\n";
    } catch (const GameException& err) {
        std::cout << "  [ok] prins GameException: " << err.what() << "\n";
    }
}

} // namespace

int main() {
    DataRegistry registry;
    try {
        registry.loadAll("data");
    } catch (const GameException& err) {
        std::cerr << "Eroare la incarcare date: " << err.what() << "\n";
        return 1;
    }

    runT2DemoTests(registry);

    std::cout << "=== EvolutionTD: Digital Immune System (SFML) ===\n";

    sf::RenderWindow window(sf::VideoMode(1920, 1080), "EvolutionTD", sf::Style::Fullscreen);
    window.setFramerateLimit(60);

    SceneManager manager;
    manager.requestPush(std::make_unique<MainMenuScene>(manager, registry));
    manager.applyPending();

    sf::Clock clock;
    while (window.isOpen() && !manager.empty()) {
        float dt = clock.restart().asSeconds();

        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                break;
            }
            manager.handleEvent(event);
        }

        manager.update(dt);

        window.clear();
        manager.render(window);
        window.display();

        // Aplica tranzitia ceruta de scene in handleEvent/update DUPA frame complet,
        // ca sa nu invalideze pointer-ul curent (top of stack).
        manager.applyPending();
    }

    return 0;
}
