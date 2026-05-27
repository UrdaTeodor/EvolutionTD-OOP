#pragma once
#include <vector>
#include <ostream>
#include <utility>
#include <memory>
#include <random>
#include <string>
#include "Tower.h"
#include "Wave.h"
#include "GlobalStatBuffs.h"
#include "Director.h"

class DataRegistry;
struct MapSpec;
struct SaveData;


// Contine grid-ul, turnurile, valul curent, HP-ul jucatorului, banii + buffs + Director.
class Game {
    static constexpr int GRID_SIZE = 20;

    // non owning pointer la DataRegistry
    const DataRegistry* registry_;

    // configuratie de baza din MapSpec (loaded in constructor)
    std::string current_map_id_;
    int  max_waves_;
    int  starting_hp_;
    int  starting_money_;

    // accumulator de buff-uri globale per TowerType. Modificat de StatEvolution::apply.
    GlobalStatBuffs buffs_;

    // RNG (folosit de Director + shop). Seedable din settings.
    // Serializable cu operator<< / operator>> pt save.
    std::mt19937 rng_;

    // Director = unlock progressiv de inamici + budget-based spawning.
    Director director_;

    char grid[GRID_SIZE][GRID_SIZE];
    bool pathGrid[GRID_SIZE][GRID_SIZE];

    std::vector<std::unique_ptr<Tower>> towers;

    Wave currentWave;
    std::vector<std::pair<int, int>> path;
    int playerHP;
    int money;
    int waveNumber;

    std::unique_ptr<Game> snapshot_;

    // Statistici pt GameOverScene + high_scores (resetate la run nou).
    int total_kills_         = 0;
    int total_money_earned_  = 0;

    // Player weight (Director scaling): incrementat la cumparare evolutii.
    int player_weight_       = 0;

    bool endless_active_     = false;

    void initPath();
    void refreshGrid();
    bool isPathCell(int x, int y) const;
    bool isOccupied(int x, int y) const;

    // Construieste val N intro din WaveSpec + Director spawns pe baza budget.
    Wave buildWave(int waveNum);

    // Apelat la endWave, deblocheaza inamici din WaveSpec.unlocks_after.
    void applyWaveUnlocks(int waveNum);

    bool isValidPlacement(int x, int y, bool needsPath) const;

public:
    // map_id ales din settings/UI. Default "default" (singura harta in T3)
    explicit Game(const DataRegistry& registry, const std::string& map_id = "default");

    // Regula celor 3 + copy-and-swap ( T2)
    Game(const Game& other);
    Game& operator=(Game other);
    ~Game();
    friend void swap(Game& a, Game& b) noexcept;

    void takeSnapshot();
    bool restoreSnapshot();
    int  getWaveNumber() const { return waveNumber; }

    // Getters pentru rendering
    const std::vector<std::unique_ptr<Tower>>& getTowers() const { return towers; }
    const Wave& getCurrentWave() const { return currentWave; }
    const std::vector<std::pair<int, int>>& getPath() const { return path; }
    int getPlayerHP() const { return playerHP; }
    int getMoney() const { return money; }

    const GlobalStatBuffs& getBuffs() const { return buffs_; }
    GlobalStatBuffs&         mutableBuffs() { return buffs_; }


    // ShopPanel reutilizeaza RNG-ul Game-ului pentru ca shop_state sa fie deterministic
    std::mt19937&  mutableRng() { return rng_; }


    // ShopPanel decrementeaza direct la cumparare (apare un check `money < cost` inainte).
    int&  mutableMoney() { return money; }

    // Statistici cumulative (resetate per run).
    int  getTotalKills()       const { return total_kills_; }
    int  getTotalMoneyEarned() const { return total_money_earned_; }
    int  getPlayerWeight()     const { return player_weight_; }
    int& mutableWeight()             { return player_weight_; }
    bool isEndlessActive()     const { return endless_active_; }
    void enableEndless()             { endless_active_ = true; }


    // Folosit de SaveManager la load (restaurare contoare).
    void setTotalKills(int v)        { total_kills_ = v; }
    void setTotalMoneyEarned(int v)  { total_money_earned_ = v; }

    const std::string& getMapId() const { return current_map_id_; }

    // Save / Load

    void serializeTo(SaveData& out) const;
    void restoreFrom(const SaveData& src);
    int  getMaxWaves()   const { return max_waves_; }

    void placeTower(int typeChoice, int x, int y);

    // Sell tower at (col, row): refund 75% cost base + 50% suma token-uri aplicate.
    // Returneaza 0 daca nu exista tower acolo.
    int  sellTower(int col, int row);

    void startWave();
    void tickWave(float dt);
    bool isWaveActive() const;
    void endWave();

    bool isGameOver() const;
    bool allWavesDone() const;

    friend std::ostream& operator<<(std::ostream& os, const Game& g);
};
