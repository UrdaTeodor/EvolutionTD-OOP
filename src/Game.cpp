#include "Game.h"

#include "AntivirusTower.h"
#include "AdblockerTower.h"
#include "HoneypotTower.h"
#include "FirewallTower.h"
#include "BytecoinMinerTower.h"
#include "DataRegistry.h"
#include "GameException.h"
#include "SaveData.h"
#include "AbilityType.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <sstream>
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
      playerHP(other.playerHP), money(other.money), waveNumber(other.waveNumber),
      total_kills_(other.total_kills_),
      total_money_earned_(other.total_money_earned_),
      player_weight_(other.player_weight_),
      endless_active_(other.endless_active_)
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

    swap(a.towers,             b.towers);
    swap(a.currentWave,        b.currentWave);
    swap(a.path,               b.path);
    swap(a.playerHP,           b.playerHP);
    swap(a.money,              b.money);
    swap(a.waveNumber,         b.waveNumber);
    swap(a.total_kills_,       b.total_kills_);
    swap(a.total_money_earned_, b.total_money_earned_);
    swap(a.player_weight_,     b.player_weight_);
    swap(a.endless_active_,    b.endless_active_);
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

    Wave wave(waveNum, {});

    const float w        = static_cast<float>(player_weight_);
    const float hp_mult  = 1.0f + 0.01f * w;
    float interval       = std::max(0.01f, 2.0f - 0.025f * std::sqrt(2.0f * w));

    if (waveNum > max_waves_) {
        int tier = std::max(0, (waveNum - max_waves_ - 1) / 10);
        float endless_mult   = std::pow(2.0f, static_cast<float>(tier));
        int   endless_budget = static_cast<int>(50.0f * endless_mult);

        auto spawns    = director_.generateSpawns(endless_budget);
        bool boss_wave = ((waveNum - max_waves_) % 15 == 0)
                      && registry_->hasEnemy("iloveyou");
        int total = static_cast<int>(spawns.size()) + (boss_wave ? 1 : 0);
        if (total > 0 && total * interval > 30.0f) {
            interval = 30.0f / static_cast<float>(total);
        }
        wave.setSpawnInterval(interval);

        if (boss_wave) {
            Enemy boss(registry_->getEnemy("iloveyou"));
            boss.scaleHealth(1.0f + (hp_mult - 1.0f) * 0.5f);
            wave.addEnemy(boss);
        }
        for (const auto& spec : spawns) {
            Enemy e(spec);
            e.scaleHealth(hp_mult);
            wave.addEnemy(e);
        }
        return wave;
    }

    const WaveSpec& wave_spec = registry_->getWave(waveIdFor(map, waveNum));

    auto spawns      = director_.generateSpawns(wave_spec.base_budget);
    int  total_spawn = static_cast<int>(wave_spec.intro.size() + spawns.size());

    // Cap 30s per wave: daca total_spawn * interval > 30, strang interval.
    if (total_spawn > 0 && total_spawn * interval > 30.0f) {
        interval = 30.0f / static_cast<float>(total_spawn);
    }
    wave.setSpawnInterval(interval);

    // Boss primeste doar jumate din buff-ul de HP (player are sansa rezonabila la wave 10).
    auto effectiveMult = [hp_mult](bool is_boss) {
        return is_boss ? (1.0f + (hp_mult - 1.0f) * 0.5f) : hp_mult;
    };

    for (const auto& key : wave_spec.intro) {
        const EnemySpec& spec = registry_->getEnemy(key);
        Enemy e(spec);
        e.scaleHealth(effectiveMult(spec.is_boss));
        wave.addEnemy(e);
    }
    for (const auto& spec : spawns) {
        Enemy e(spec);
        e.scaleHealth(effectiveMult(spec.is_boss));
        wave.addEnemy(e);
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

    int count_of_type = 0;
    for (const auto& t : towers) if (t->getTypeKey() == key) ++count_of_type;
    if (count_of_type >= spec.max_count) {
        throw InvalidPlacementException(
            "Limita atinsa pentru " + spec.display_name +
            " (max " + std::to_string(spec.max_count) + ")");
    }

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

int Game::sellTower(int col, int row) {
    for (auto it = towers.begin(); it != towers.end(); ++it) {
        if ((*it)->getX() == col && (*it)->getY() == row) {
            int refund = static_cast<int>(0.75f * (*it)->getCost())
                       + static_cast<int>(0.5f  * (*it)->getTokenInvestment());
            money += refund;
            towers.erase(it);
            refreshGrid();
            return refund;
        }
    }
    return 0;
}

void Game::startWave() {
    takeSnapshot();
    currentWave = buildWave(waveNumber);
}

void Game::tickWave(float dt) {
    if (!isWaveActive()) return;
    int earned = 0;
    int killed = 0;
    int damage = currentWave.simulate(towers, path, dt, earned, killed, buffs_);
    money    += earned;
    playerHP -= damage;
    total_kills_        += killed;
    total_money_earned_ += earned;
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

bool Game::isGameOver() const {
    return playerHP <= 0;
}

bool Game::allWavesDone() const {
    return waveNumber > max_waves_ && !endless_active_;
}

std::ostream& operator<<(std::ostream& os, const Game& g) {
    int displayWave = g.waveNumber <= g.max_waves_ ? g.waveNumber : g.max_waves_;
    os << "[ System HP: " << g.playerHP
       << " | Credits: " << g.money
       << " | Wave: " << displayWave << "/" << g.max_waves_
       << " | Enemies on field: " << g.currentWave.activeCount() << " ]\n";
    return os;
}


// Save / Load


namespace {
    int keyToTypeChoice(const std::string& key) {
        if (key == "antivirus")     return 1;
        if (key == "adblocker")     return 2;
        if (key == "honeypot")      return 3;
        if (key == "firewall")      return 4;
        if (key == "bytecoinminer") return 5;
        return 0;
    }
}

void Game::serializeTo(SaveData& out) const {
    out.map_id             = current_map_id_;
    out.wave_number        = waveNumber;
    out.player_hp          = playerHP;
    out.money              = money;
    out.total_kills        = total_kills_;
    out.total_money_earned = total_money_earned_;
    out.player_weight      = player_weight_;
    out.endless_active     = endless_active_;

    out.towers.clear();
    for (const auto& t : towers) {
        SaveData::TowerEntry te;
        te.type_key         = t->getTypeKey();
        te.col              = t->getX();
        te.row              = t->getY();
        te.token_investment = t->getTokenInvestment();
        for (AbilityType ab : t->getAppliedAbilities()) {
            te.applied_abilities.emplace_back(abilityToString(ab));
        }
        out.towers.push_back(std::move(te));
    }

    // Buffs
    out.buffs.clear();
    for (const auto& [type_key, tb] : buffs_.all()) {
        SaveData::BuffEntry be;
        be.type_key         = type_key;
        be.damage_pct       = tb.damage_pct;
        be.range_pct        = tb.range_pct;
        be.attack_speed_pct = tb.attack_speed_pct;
        be.max_hp_pct       = tb.max_hp_pct;
        be.regen_pct        = tb.regen_pct;
        be.slow_pct         = tb.slow_pct;
        be.income_pct       = tb.income_pct;
        out.buffs.push_back(std::move(be));
    }

    // RNG state: operator<< standard pe mt19937 (text serialization).
    std::ostringstream oss;
    oss << rng_;
    out.rng_state = oss.str();
}

void Game::restoreFrom(const SaveData& src) {
    current_map_id_     = src.map_id;
    waveNumber          = src.wave_number;
    playerHP            = src.player_hp;
    total_kills_        = src.total_kills;
    total_money_earned_ = src.total_money_earned;
    player_weight_      = src.player_weight;
    endless_active_     = src.endless_active;

    // Buffs: rebuild
    buffs_ = GlobalStatBuffs{};
    for (const auto& be : src.buffs) {
        auto& tb = buffs_.mutable_for(be.type_key);
        tb.damage_pct       = be.damage_pct;
        tb.range_pct        = be.range_pct;
        tb.attack_speed_pct = be.attack_speed_pct;
        tb.max_hp_pct       = be.max_hp_pct;
        tb.regen_pct        = be.regen_pct;
        tb.slow_pct         = be.slow_pct;
        tb.income_pct       = be.income_pct;
    }

    // Towers: clear si replay placeTower cu money bypass (placeTower deduce cost,
    // dam temporar money infinit ca sa nu blocheze; resetam money la final).
    towers.clear();
    int saved_money = src.money;
    money = 1000000000;
    for (const auto& te : src.towers) {
        int choice = keyToTypeChoice(te.type_key);
        if (choice == 0) continue;
        try {
            placeTower(choice, te.col, te.row);
        } catch (const GameException&) {
            continue;
        }
        Tower* placed = towers.back().get();
        placed->recordTokenInvestment(te.token_investment);
        // Apply abilities. KNOCKBACK_EVERY_3 e cazul special cu dynamic_cast (T2 pastrat).
        for (const auto& ab_str : te.applied_abilities) {
            if (ab_str == "KNOCKBACK_EVERY_3") {
                if (auto* anti = dynamic_cast<AntivirusTower*>(placed)) {
                    anti->setKnockbackInterval(3);
                } else if (auto* adb = dynamic_cast<AdblockerTower*>(placed)) {
                    adb->setKnockbackInterval(3);
                }
                continue;
            }
            try {
                AbilityType ab = stringToAbility(ab_str);
                placed->applyAbility(ab);
            } catch (const std::exception&) {
                // necunoscut / incompatibil - skip
            }
        }
    }
    money = saved_money;

    // RNG deserialize
    if (!src.rng_state.empty()) {
        std::istringstream iss(src.rng_state);
        iss >> rng_;
    }
    // Director re-aplica unlocks pentru wave-urile deja trecute
    director_ = Director(rng_);
    for (int w = 1; w < waveNumber; w++) {
        applyWaveUnlocks(w);
    }

    refreshGrid();
}
