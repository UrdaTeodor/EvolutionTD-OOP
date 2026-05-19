#include "Game.h"

#include "AntivirusTower.h"
#include "AdblockerTower.h"
#include "HoneypotTower.h"
#include "FirewallTower.h"
#include "GameException.h"
#include <iostream>
#include <iomanip>     
#include <cmath>       
#include <algorithm>   
#include <string>      

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

// Constructor de COPIERE (polimorfic)
// Game detine vector<unique_ptr<Tower>>: cc-ul implicit ar fi sters (unique_ptr).
// Aici facem manual deep-copy clonand fiecare turn cu Tower::clone() (apel polimorfic
// prin pointer la baza -> returneaza unique_ptr<Tower> de tipul concret corect).
Game::Game(const Game& other)
    : currentWave(other.currentWave),
      playerHP(other.playerHP), money(other.money), waveNumber(other.waveNumber)
      // snapshot_ ramane nullptr 
{
    // grid si pathGrid: initPath() le reconstruieste din hardcoded path.
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            grid[row][col]     = '.';
            pathGrid[row][col] = false;
        }
    }
    initPath();

    towers.reserve(other.towers.size());
    for (const auto& t : other.towers) {
        towers.push_back(t->clone());
    }

    refreshGrid();   
}

// Operator= cu copy-and-swap 
// Param by-value -> cc-ul s-a apelat deja inainte sa intram in functie.
// Singura operatiune ramasa e swap-ul, care e noexcept
Game& Game::operator=(Game other) {
    swap(*this, other);
    refreshGrid();  
    return *this;
}

// Destructor explicit
// Necesar pt ca avem unique_ptr<Game> snapshot_ self-referential:
Game::~Game() = default;

// Friend swap 
// nu swap-uim: snapshot_ , grid , pathGrid si path .
void swap(Game& a, Game& b) noexcept {
    using std::swap;
    swap(a.towers,      b.towers);
    swap(a.currentWave, b.currentWave);
    swap(a.playerHP,    b.playerHP);
    swap(a.money,       b.money);
    swap(a.waveNumber,  b.waveNumber);
}

// Snapshot / restore 
// takeSnapshot: salvam o copie a *this. Folosim cc-ul nostru (deep-copy prin clone()).
// snapshot_-ul copiei va fi nullptr (cc-ul nu copiaza snapshot_).
void Game::takeSnapshot() {
    snapshot_ = std::make_unique<Game>(*this);
}

// restoreSnapshot: reincarcam state-ul din snapshot prin op=.
// dupa restore undo se poate aplica de cate ori vrem.
bool Game::restoreSnapshot() {
    if (!snapshot_) return false;
    *this = *snapshot_;
    return true;
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
            if (pathGrid[row][col]) {
                grid[row][col] = 'P';
            } else {
                grid[row][col] = '.';
            }
        }
    }
    // strat 2: turnurile. APEL POLIMORFIC: tower->getDisplayChar() cheama varianta
    // corecta in functie de tipul concret ('A' pt Antivirus, 'F' pt Firewall, etc.)
    for (const auto& tower : towers) {
        grid[tower->getY()][tower->getX()] = tower->getDisplayChar();
    }
    // strat 3: inamicii (suprascriu tot)
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

// Itereaza prin turnuri si verifica daca exista deja unul la (col, row).
// tower-> pentru ca elementele din vector sunt unique_ptr<Tower>.
bool Game::isOccupied(int col, int row) const {
    for (const auto& tower : towers) {
        if (tower->getX() == col && tower->getY() == row) return true;
    }
    return false;
}

// Verifica plasarea: in grid, neocupat, si pe path (sau in afara) in functie de needsPath.
bool Game::isValidPlacement(int col, int row, bool needsPath) const {
    if (col < 0 || col >= GRID_SIZE || row < 0 || row >= GRID_SIZE) return false;
    if (isOccupied(col, row)) return false;
    bool onPath = isPathCell(col, row);
    return needsPath ? onPath : !onPath;
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

// Cumpara si plaseaza un turn de tipul typeChoice la (col, row).
// exceptii :
//   - InvalidPlacementException pentru orice eroare de plasare
//   - InsufficientFundsException cand nu ai destui credit
void Game::placeTower(int typeChoice, int col, int row) {
    // construim turnul prin factory (returneaza unique_ptr<Tower>).
    // Daca aruncam exceptie dupa, unique_ptr-ul iese din scope si turnul se sterge
    std::unique_ptr<Tower> newTower;
    switch (typeChoice) {
        case 1: newTower = makeAntivirus(col, row); break;   // -> AntivirusTower.cpp
        case 2: newTower = makeAdblocker(col, row); break;   // -> AdblockerTower.cpp
        case 3: newTower = makeHoneypot(col, row);  break;   // -> HoneypotTower.cpp
        case 4: newTower = makeFirewall(col, row);  break;   // -> FirewallTower.cpp
        default:
            throw InvalidPlacementException(
                "Tip turn invalid: " + std::to_string(typeChoice) + " (alege 1-4)");
    }

    // Apel polymorphic: requiresPath() returneaza true doar pt FirewallTower.
    bool needsPath = newTower->requiresPath();

    if (!isValidPlacement(col, row, needsPath)) {
        std::string pos = "(" + std::to_string(col) + "," + std::to_string(row) + ")";
        if (isOccupied(col, row)) {
            throw InvalidPlacementException("Celula " + pos + " e deja ocupata");
        }
        if (needsPath && !isPathCell(col, row)) {
            throw InvalidPlacementException(
                "Firewall trebuie plasat PE drum, dar " + pos + " nu e drum");
        }
        throw InvalidPlacementException(
            "Celula " + pos + " e pe drum (doar Firewall poate fi acolo)");
    }

    int towerCost = newTower->getCost();
    if (money < towerCost) {
        throw InsufficientFundsException(towerCost, money);
    }

    money -= towerCost;
    std::cout << "Placed " << newTower->getName()
              << " at (row=" << row << ", col=" << col << ") for " << towerCost << " credits.\n";
    // std::move = transferam proprietatea unique_ptr-ului catre vector.
    towers.push_back(std::move(newTower));
    refreshGrid();
}

// Ruleaza valul curent: loop pe tick-uri pana cand toti inamicii sunt morti/scapati
// sau jucatorul ramane fara HP.
void Game::runWave() {
    takeSnapshot();

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

// Friend operator<<: afiseaza statusul jocului (HP, bani, val curent).
std::ostream& operator<<(std::ostream& os, const Game& g) {
    // limitam display-ul la MAX_WAVES (ca sa nu apara "4/3" cand am terminat valul 3)
    int displayWave = g.waveNumber <= Game::MAX_WAVES ? g.waveNumber : Game::MAX_WAVES;
    os << "[ System HP: " << g.playerHP
       << " | Credits: " << g.money
       << " | Wave: " << displayWave << "/" << Game::MAX_WAVES
       << " | Enemies on field: " << g.currentWave.activeCount() << " ]\n";
    return os;
}
