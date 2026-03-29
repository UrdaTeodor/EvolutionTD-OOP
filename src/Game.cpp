#include "Game.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>

// ---- constructor ----

Game::Game()
    : currentWave(0, {}), // val initial temporar
      playerHP(STARTING_HP), money(STARTING_MONEY), waveNumber(1) {
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            grid[row][col]     = '.';
            pathGrid[row][col] = false;
        }
    }
    initPath();
    refreshGrid();
}

// ---- private helpers ----

// Drum: (linie, coloana). Segmentele sunt paralele cu axele.
// (5,0) -> (5,10) -> (14,10) -> (14,19)
void Game::initPath() {
    path = {{5,0}, {5,10}, {14,10}, {14,19}};

    for (int i = 0; i < static_cast<int>(path.size()) - 1; i++) {
        int row1 = path[i].first,   col1 = path[i].second;
        int row2 = path[i+1].first, col2 = path[i+1].second;

        if (row1 == row2) {
            
            for (int col = std::min(col1, col2); col <= std::max(col1, col2); col++) {
                pathGrid[row1][col] = true;
            }
        } else {
            for (int row = std::min(row1, row2); row <= std::max(row1, row2); row++) {
                pathGrid[row][col1] = true;
            }
        }
    }
}

void Game::refreshGrid() {
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            if (pathGrid[row][col]) 
            {
                grid[row][col] = 'P';
            } 
            else 
            {
                grid[row][col] = '.';
            }
        }
    }
    // Stratul de turnuri (suprascrie caracterul de drum pentru Firewall, care sta pe drum)
    for (const auto& tower : towers) {
        char c = '?';
        switch (tower.getType()) {
            case TowerType::ANTIVIRUS: c = 'A'; break;
            case TowerType::ADBLOCKER: c = 'D'; break;
            case TowerType::HONEYPOT:  c = 'H'; break;
            case TowerType::FIREWALL:  c = 'F'; break;
        }
        grid[tower.getY()][tower.getX()] = c;
    }
    // Stratul inamicilor (suprascrie tot)
    for (const auto& enemy : currentWave.getActiveEnemies()) {
        if (!enemy.isAlive()) continue;
        int col = static_cast<int>(std::round(enemy.getX()));
        int row = static_cast<int>(std::round(enemy.getY()));
        if (col >= 0 && col < GRID_SIZE && row >= 0 && row < GRID_SIZE) {
            grid[row][col] = 'E';
        }
    }
}

bool Game::isPathCell(int col, int row) const {
    return pathGrid[row][col];
}

bool Game::isOccupied(int col, int row) const {
    for (const auto& tower : towers) {
        if (tower.getX() == col && tower.getY() == row) return true;
    }
    return false;
}


//   Firewall trebuie sa fie pe drum si neocupat
bool Game::isValidPlacement(int col, int row, TowerType type) const {
    if (col < 0 || col >= GRID_SIZE || row < 0 || row >= GRID_SIZE) return false;
    if (isOccupied(col, row)) return false;
    bool onPath = isPathCell(col, row);
    return (type == TowerType::FIREWALL) ? onPath : !onPath;
}

Wave Game::buildWave(int waveNum) {
    Wave wave(waveNum, {});
    switch (waveNum) {
        case 1:
            wave.addEnemy(makeAdware());
            wave.addEnemy(makeAdware());
            wave.addEnemy(makeAdware());
            break;
        case 2:
            wave.addEnemy(makeAdware());
            wave.addEnemy(makeAdware());
            wave.addEnemy(makeTrojan());
            wave.addEnemy(makeTrojan());
            break;
        case 3:
            // worm: momentan HP mic + viteza mare, in viitor va da split in 2 cand moare
            wave.addEnemy(makeWorm());
            wave.addEnemy(makeWorm());
            wave.addEnemy(makeWorm());
            wave.addEnemy(makeTrojan());
            break;
        default: break;
    }
    return wave;
}


bool Game::placeTower(int typeChoice, int col, int row) {
    TowerType type;
    switch (typeChoice) {
        case 1: type = TowerType::ANTIVIRUS; break;
        case 2: type = TowerType::ADBLOCKER; break;
        case 3: type = TowerType::HONEYPOT;  break;
        case 4: type = TowerType::FIREWALL;  break;
        default:
            std::cout << "Invalid tower type (1-4).\n";
            return false;
    }

    if (!isValidPlacement(col, row, type)) {
        std::cout << "Cannot place tower at (" << col << "," << row << "): ";
        if (isOccupied(col, row)) {
            std::cout << "cell is already occupied.\n";
        } else if (type == TowerType::FIREWALL && !isPathCell(col, row)) {
            std::cout << "Firewall must be placed ON the path.\n";
        } else {
            std::cout << "cell is on the path (only Firewall may go there).\n";
        }
        return false;
    }

    // Construim turnul in functie de tip 
    Tower newTower = makeAntivirus(col, row); // implicit; suprascris mai jos daca e nevoie
    if      (type == TowerType::ADBLOCKER) newTower = makeAdblocker(col, row);
    else if (type == TowerType::HONEYPOT)  newTower = makeHoneypot(col, row);
    else if (type == TowerType::FIREWALL)  newTower = makeFirewall(col, row);

    int towerCost = newTower.getCost();
    if (money < towerCost) {
        std::cout << "Not enough credits (need " << towerCost << ", have " << money << ").\n";
        return false;
    }

    money -= towerCost;
    towers.push_back(newTower);
    refreshGrid();
    std::cout << "Placed " << newTower.getName()
              << " at (" << col << "," << row << ") for " << towerCost << " credits.\n";
    return true;
}

void Game::runWave() {
    currentWave = buildWave(waveNumber);

    std::cout << "\n=== WAVE " << waveNumber << " starting ===\n";
    std::cout << currentWave;

    constexpr float DT           = 0.1f;  // pasul de simulare in secunde
    constexpr float PRINT_EVERY  = 2.0f;  // afisam grila din N in N secunde
    float timeSincePrint = PRINT_EVERY;   // afisam imediat la prima iteratie
    int   safetyLimit    = 5000;          // numar maxim de tick-uri (~500 secunde) pentru a preveni o bucla infinita

    for (int tick = 0; tick < safetyLimit && !currentWave.allDefeated() && !isGameOver(); tick++) {
        int earned = 0;
        int damage = currentWave.simulate(towers, path, DT, earned);

        if (earned > 0) {
            money    += earned;
            std::cout << "  >> +" << earned << " credits earned! Total: " << money << "\n";
        }
        playerHP -= damage;

        timeSincePrint += DT;
        if (timeSincePrint >= PRINT_EVERY) {
            refreshGrid();
            displayGrid();
            std::cout << *this;
            timeSincePrint = 0.0f;
        }
    }

    refreshGrid();
    displayGrid();
    std::cout << "\n=== WAVE " << waveNumber << " complete ===\n";
    std::cout << *this;
    waveNumber++;
}

void Game::displayGrid() const {
    // Antet coloana
    std::cout << "\n   ";
    for (int col = 0; col < GRID_SIZE; col++) {
        std::cout << (col % 10);
    }
    std::cout << "\n";
    // Linii
    for (int row = 0; row < GRID_SIZE; row++) {
        std::cout << std::setw(2) << row << " ";
        for (int col = 0; col < GRID_SIZE; col++) {
            std::cout << grid[row][col];
        }
        std::cout << "\n";
    }
    std::cout << "Legend: .=empty  P=path  A=Antivirus  D=Adblocker  H=Honeypot  F=Firewall  E=Enemy\n";
}

bool Game::isGameOver() const {
    return playerHP <= 0;
}

bool Game::allWavesDone() const {
    return waveNumber > MAX_WAVES;
}

std::ostream& operator<<(std::ostream& os, const Game& g) {
    int displayWave = g.waveNumber <= Game::MAX_WAVES ? g.waveNumber : Game::MAX_WAVES;
    os << "[ System HP: " << g.playerHP
       << " | Credits: " << g.money
       << " | Wave: " << displayWave << "/" << Game::MAX_WAVES
       << " | Enemies on field: " << g.currentWave.activeCount() << " ]\n";
    return os;
}
