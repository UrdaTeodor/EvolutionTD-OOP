#include <iostream>
#include <string>
#include "Game.h"
#include "Wave.h"
#include "Enemy.h"
#include "GameException.h"
#include "EvolutionFactory.h"
#include "AbilityEvolution.h"
#include "AntivirusTower.h"

// EvolutionTD - digital system tower defense
//
// Path: (row, col):  (5,0) -> (5,10) -> (14,10) -> (14,19)
//
// Turnuri disponibile:
//   1 = PrimitiveAV     - $50, off-path, medium damage, 1 shot/s, range 4
//   2 = Adblocker       - $40, off-path, low damage, 4 shots/s, range 3
//   3 = Honeypot        - $30, off-path, slows enemies 20% in range 1.5
//   4 = Firewall        - $60, on-path, absorbs enemies cu HP <= HP propriu
//   5 = BytecoinMiner   - $50, off-path, nu ataca, +25 credits/val (ROI 2 valuri)
//
// tastatura.txt format:
//   <type> <row> <col>    plaseaza un tower
//   $                     porneste valul urmator

int main() {
    Game game;

    // Test Regula celor 3 pentru Wave:
    {
        Wave w1(0, {});
        w1.addEnemy(makeAdware());      // w1: 1 inamic

        Wave w2 = w1;                   // constructor de copiere
        w2.addEnemy(makeTrojan());      // modificam COPIA -> w1 nu trebuie afectat

        Wave w3(0, {});
        w3 = w1;                        // operator= de copiere
        w3.addEnemy(makeWorm());        // modificam si w3 -> w1 tot neatins

        std::cout << "=== Test Regula celor 3 ===\n";
        std::cout << "w1 original (1 inamic):         " << w1;
        std::cout << "w2 copie cc (2 inamici):        " << w2;
        std::cout << "w3 copie op= (2 inamici):       " << w3;
        std::cout << "=> w1 are tot 1 inamic: copiile sunt independente\n";
    }

    // Test ierarhie proprie de exceptii
    // Aratam ca toate cele 3 tipuri sunt aruncate si prinse corect.
    // Folosim catch pe ierarhia comuna GameException -> un singur catch
    // prinde toate exceptiile noastre indiferent de tip.
    std::cout << "=== Test excepții (GameException + 3 derivate) ===\n";

    // 1. IncompatibleEvolutionException din craftMythic
    //    (combo invalid: DoubleShot + ReflectiveShield nu se combina)
    try {
        auto a = std::make_unique<AbilityEvolution>(
            "DoubleShot", 200, Evolution::Rarity::LEGENDARY,
            AbilityEvolution::AbilityType::DOUBLE_SHOT);
        auto b = std::make_unique<AbilityEvolution>(
            "ReflectShield", 200, Evolution::Rarity::LEGENDARY,
            AbilityEvolution::AbilityType::REFLECTIVE_SHIELD);
        craftMythic(std::move(a), std::move(b));
        std::cout << "  [BUG] throw\n";
    } catch (const IncompatibleEvolutionException& err) {
        std::cout << "  [ok] prins IncompatibleEvolutionException: " << err.what() << "\n";
    }

    // 2. IncompatibleEvolutionException din AbilityEvolution::apply
    //    (BiggerAura aplicat pe AntivirusTower in loc de Honeypot)
    try {
        AntivirusTower anti(0, 0);
        AbilityEvolution wrong("EpicBiggerAura", 100, Evolution::Rarity::EPIC,
                               AbilityEvolution::AbilityType::BIGGER_AURA);
        wrong.apply(anti);
        std::cout << "  [BUG] throw\n";
    } catch (const GameException& err) {  
        std::cout << "  [ok] prins GameException: " << err.what() << "\n";
    }

    std::cout << "=== EvolutionTD: Digital Immune System ===\n";
    game.displayGrid();
    std::cout << game;

    // Loop principal: cat timp mai sunt valuri si jucatorul nu a pierdut.
    // wave-ul local e doar pt afisarea header-ului; logica reala foloseste
    // game.getWaveNumber() (poate scadea daca user da undo).
    for (int wave = 1; !game.allWavesDone() && !game.isGameOver(); wave++) {
        std::cout << "\n--- Pre-wave " << wave << " placement phase ---\n";
        std::cout << game;
        std::cout << "Towers: 1=PrimitiveAV($50) 2=Adblocker($40) 3=Honeypot($30) 4=Firewall($60,path) 5=Miner($50,+25/wave)\n";
        std::cout << "Enter placements (type row col). 'u'=undo last wave. '$'=start wave:\n";

        // Citim placement-urile pana la "$" (separator de val).
        // placeTower poate arunca InvalidPlacementException sau InsufficientFundsException
        // "u" = undo: restore la snapshot-ul de la inceputul ultimului val
        std::string tok;
        while (std::cin >> tok && tok != "$") {
            if (tok == "u") {
                if (game.restoreSnapshot()) {
                    wave = game.getWaveNumber();   
                    std::cout << ">>> Undo: revenit la start val " << wave << "\n";
                    std::cout << game;
                } else {
                    std::cout << "  !! Nu exista snapshot (probabil wave 1)\n";
                }
                continue;
            }
            int row = 0, col = 0;
            std::cin >> row >> col;
            try {
                game.placeTower(std::stoi(tok), col, row);
            } catch (const GameException& err) {
                // Un singur catch pe baza prinde si InvalidPlacementException
                // si InsufficientFundsException
                std::cout << "  !! " << err.what() << "\n";
            }
        }

        game.displayGrid();
        game.runWave();

        if (game.isGameOver()) {
            std::cout << "\n!!! SYSTEM COMPROMISED - GAME OVER !!!\n";
        }
    }

    if (!game.isGameOver()) {
        std::cout << "\n=== All waves defeated - System secured! ===\n";
        std::cout << game;
    }

    return 0;
}
