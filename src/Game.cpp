#include "Game.h"

#include "AntivirusTower.h"
#include "AdblockerTower.h"
#include "HoneypotTower.h"
#include "FirewallTower.h"
#include "BytecoinMinerTower.h"
#include "DataRegistry.h"
#include "GameException.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <string>

namespace {
    // Lookup wave_id pentru indexul waveNumber (1-based).
    const std::string& waveIdFor(const MapSpec& map, int waveNumber) {
        if (waveNumber < 1 || waveNumber > static_cast<int>(map.wave_ids.size())) {
            throw DataException("waveNumber " + std::to_string(waveNumber) +
                                " in afara range pentru map '" + map.name + "'");
        }
        return map.wave_ids[waveNumber - 1];
    }
}

// ---- constructor ----

Game::Game(const DataRegistry& registry, const std::string& map_id)
    : registry_(&registry),
      current_map_id_(map_id),
      max_waves_(0),
      starting_hp_(0),
      starting_money_(0),
      buffs_(),
      rng_(std::random_device{}()),     // default seed (random); save_load suprascrie
      director_(rng_),
      currentWave(0, {}),
      playerHP(0), money(0), waveNumber(1) {
    const MapSpec& map = registry_->getMap(map_id);
    max_waves_      = static_cast<int>(map.wave_ids.size());
    starting_hp_    = map.start_hp;
    starting_money_ = map.start_money;
    playerHP        = starting_hp_;
    money           = starting_money_;

    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            grid[row][col]     = '.';
            pathGrid[row][col] = false;
        }
    }
    path = map.path;
    initPath();
    refreshGrid();
}

Game::Game(const Game& other)
    : registry_(other.registry_),
      current_map_id_(other.current_map_id_),
      max_waves_(other.max_waves_),
      starting_hp_(other.starting_hp_),
      starting_money_(other.starting_money_),
      buffs_(other.buffs_),
      rng_(other.rng_),               // copiaza state-ul RNG (reproducible)
      director_(other.director_),     // copy default pool_ + rng_ pointer 
      currentWave(other.currentWave),
      path(other.path),
      playerHP(other.playerHP), money(other.money), waveNumber(other.waveNumber)
{
    // Director.rng_ pointer din other puncteaza la &other.rng_
    director_.setRng(rng_);

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

Game& Game::operator=(Game other) {
    swap(*this, other);
    refreshGrid();
    return *this;
}

Game::~Game() = default;

void swap(Game& a, Game& b) noexcept {
    using std::swap;
    swap(a.registry_,       b.registry_);
    swap(a.current_map_id_, b.current_map_id_);
    swap(a.max_waves_,      b.max_waves_);
    swap(a.starting_hp_,    b.starting_hp_);
    swap(a.starting_money_, b.starting_money_);
    swap(a.buffs_,          b.buffs_);
    swap(a.rng_,            b.rng_);
    swap(a.director_,       b.director_);
    // Director.rng_ pointer-ul s-a swap-uit gresit (pointeaza la celalalt Game)
    a.director_.setRng(a.rng_);
    b.director_.setRng(b.rng_);

    swap(a.towers,      b.towers);
    swap(a.currentWave, b.currentWave);
    swap(a.path,        b.path);
    swap(a.playerHP,    b.playerHP);
    swap(a.money,       b.money);
    swap(a.waveNumber,  b.waveNumber);
}

void Game::takeSnapshot() {
    snapshot_ = std::make_unique<Game>(*this);
}

bool Game::restoreSnapshot() {
    if (!snapshot_) return false;
    *this = *snapshot_;
    return true;
}

// ---- private helpers ----

// path-ul vine acum din MapSpec; ce face initPath e construirea pathGrid.
void Game::initPath() {
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
    for (const auto& tower : towers) {
        grid[tower->getY()][tower->getX()] = tower->getDisplayChar();
    }
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
        if (tower->getX() == col && tower->getY() == row) return true;
    }
    return false;
}

bool Game::isValidPlacement(int col, int row, bool needsPath) const {
    if (col < 0 || col >= GRID_SIZE || row < 0 || row >= GRID_SIZE) return false;
    if (isOccupied(col, row)) return false;
    bool onPath = isPathCell(col, row);
    return needsPath ? onPath : !onPath;
}


// intro hardcodat din WaveSpec + Director procedural.
Wave Game::buildWave(int waveNum) {
    const MapSpec&  map  = registry_->getMap(current_map_id_);
    const WaveSpec& wave_spec = registry_->getWave(waveIdFor(map, waveNum));

    Wave wave(waveNum, {});

    //  intro din JSON
    for (const auto& key : wave_spec.intro) {
        wave.addEnemy(Enemy(registry_->getEnemy(key)));
    }

    //  Director: spawnari random ponderate (din buget)
    auto spawns = director_.generateSpawns(wave_spec.base_budget);
    for (const auto& spec : spawns) {
        wave.addEnemy(Enemy(spec));
    }

    return wave;
}

void Game::applyWaveUnlocks(int waveNum) {
    const MapSpec&  map = registry_->getMap(current_map_id_);
    if (waveNum < 1 || waveNum > static_cast<int>(map.wave_ids.size())) return;
    const WaveSpec& wave_spec = registry_->getWave(waveIdFor(map, waveNum));
    for (const auto& key : wave_spec.unlocks_after) {
        director_.unlock(key, registry_->getEnemy(key));
    }
}

void Game::placeTower(int typeChoice, int col, int row) {
    static const char* keys[] = {
        "antivirus", "adblocker", "honeypot", "firewall", "bytecoinminer"
    };
    if (typeChoice < 1 || typeChoice > 5) {
        throw InvalidPlacementException(
            "Tip turn invalid: " + std::to_string(typeChoice) + " (alege 1-5)");
    }
    const std::string& key = keys[typeChoice - 1];
    const TowerSpec& spec = registry_->getTower(key);

    std::unique_ptr<Tower> newTower;
    switch (typeChoice) {
        case 1: newTower = makeAntivirus(spec, col, row); break;
        case 2: newTower = makeAdblocker(spec, col, row); break;
        case 3: newTower = makeHoneypot(spec, col, row);  break;
        case 4: newTower = makeFirewall(spec, col, row);  break;
        case 5: newTower = makeBytecoinMiner(spec, col, row); break;
    }

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
    towers.push_back(std::move(newTower));
    refreshGrid();
}

void Game::startWave() {
    takeSnapshot();
    currentWave = buildWave(waveNumber);
}

void Game::tickWave(float dt) {
    if (!isWaveActive()) return;
    int earned = 0;
    int damage = currentWave.simulate(towers, path, dt, earned, buffs_);
    money    += earned;
    playerHP -= damage;
}

bool Game::isWaveActive() const {
    return !currentWave.allDefeated() && !isGameOver();
}

void Game::endWave() {
    // venit pasiv (Miner returneaza income, restul 0)
    for (const auto& tower : towers) {
        money += tower->collectIncome(buffs_);
    }
    // Aplica unlocks din WaveSpec pentru wave
    applyWaveUnlocks(waveNumber);
    waveNumber++;
}

// cppcheck-suppress unusedFunction
void Game::runWave() {
    takeSnapshot();
    currentWave = buildWave(waveNumber);

    std::cout << "\n=== WAVE " << waveNumber << " starting ===\n";
    std::cout << currentWave;

    constexpr float DT           = 0.1f;
    constexpr float PRINT_EVERY  = 2.0f;
    float timeSincePrint = PRINT_EVERY;
    int   safetyLimit    = 5000;

    for (int tick = 0; tick < safetyLimit && !currentWave.allDefeated() && !isGameOver(); tick++) {
        int earned = 0;
        int damage = currentWave.simulate(towers, path, DT, earned, buffs_);

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

    for (const auto& tower : towers) {
        int income = tower->collectIncome(buffs_);
        if (income > 0) {
            money += income;
            std::cout << "  >> " << tower->getName() << " mined +" << income
                      << " credits! Total: " << money << "\n";
        }
    }

    std::cout << *this;
    applyWaveUnlocks(waveNumber);
    waveNumber++;
}

void Game::displayGrid() const {
    std::cout << "\n   ";
    for (int col = 0; col < GRID_SIZE; col++) {
        std::cout << (col % 10);
    }
    std::cout << "\n";
    for (int row = 0; row < GRID_SIZE; row++) {
        std::cout << std::setw(2) << row << " ";
        for (int col = 0; col < GRID_SIZE; col++) {
            std::cout << grid[row][col];
        }
        std::cout << "\n";
    }
    std::cout << "Legend: .=empty  P=path  A=Antivirus  D=Adblocker  H=Honeypot  F=Firewall  M=Miner  E=Enemy\n";
}

bool Game::isGameOver() const {
    return playerHP <= 0;
}

bool Game::allWavesDone() const {
    return waveNumber > max_waves_;
}

std::ostream& operator<<(std::ostream& os, const Game& g) {
    int displayWave = g.waveNumber <= g.max_waves_ ? g.waveNumber : g.max_waves_;
    os << "[ System HP: " << g.playerHP
       << " | Credits: " << g.money
       << " | Wave: " << displayWave << "/" << g.max_waves_
       << " | Enemies on field: " << g.currentWave.activeCount() << " ]\n";
    return os;
}
