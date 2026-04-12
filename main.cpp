#include <iostream>
#include <string>
#include "Game.h"
#include "Wave.h"
#include "Enemy.h"

// EvolutionTD - digitalsystem tower defense
//
// Path: (row, col):  (5,0) -> (5,10) -> (14,10) -> (14,19)
//
// 
//   1 = PrimitiveAV  - $50, off-path, medium damage, 1 shot/s, range 4
//   2 = Adblocker    - $40, off-path, low damage, 4 shots/s, range 3
//   3 = Honeypot     - $30, off-path, slows enemies 20% in range 1.5
//   4 = Firewall     - $60, on-path, absorbs enemies with HP <= its own HP, ignores rest.
//
// tastatura.txt format:
//   <type> <row> <col>    place a tower 
//   $                     start the next wave

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

    std::cout << "=== EvolutionTD: Digital Immune System ===\n";
    game.displayGrid();
    std::cout << game;

    for (int wave = 1; !game.allWavesDone() && !game.isGameOver(); wave++) {
        std::cout << "\n--- Pre-wave " << wave << " placement phase ---\n";
        std::cout << game;
        std::cout << "Towers: 1=PrimitiveAV($50) 2=Adblocker($40) 3=Honeypot($30) 4=Firewall($60,path)\n";
        /////////////////////////////////////////////////////////////////////////
        /// Observatie: dati exemple de date de intrare in fisierul tastatura.txt
        /// astfel incat executia programului sa se incheie.
        /// Format: <type> <row> <col> pentru fiecare turn dorit,
        ///         apoi $ pe o linie separata pentru a incepe valul urmator.
        /// Pe GitHub Actions, fisierul tastatura.txt simuleaza intrarea de la tastatura.
        /////////////////////////////////////////////////////////////////////////
        std::cout << "Enter placements (type row col). Enter $ to start the wave:\n";

        std::string tok;
        while (std::cin >> tok && tok != "$") {
            int row = 0, col = 0;
            std::cin >> row >> col;
            game.placeTower(std::stoi(tok), col, row);
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
//.