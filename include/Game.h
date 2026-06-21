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
#include "VisualEvent.h"

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

    // Evenimentul de loterie (v0.4.1): popup scam-ad oferit inainte de valurile
    // 9 si 12. Accept = "ruta hardcore": 6 valuri cu HP boostat (doar HP, nu
    // numar de inamici), la final bani + Mythic Token; boss-ul final e buffat.
    int  lottery_offers_made_   = 0;
    bool lottery_accepted_      = false;
    bool lottery_offer_pending_ = false;
    int  gauntlet_waves_left_   = 0;
    bool gauntlet_reward_ready_ = false;

    // Venitul per-Miner din ultimul endWave, pana il consuma GameScene
    // (monedele care zboara spre contor). Transient, dar copiat la snapshot
    // ca regula celor 3 sa ramana completa.
    std::vector<IncomeEvent> income_events_;

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
    // cppcheck-suppress unusedFunction
    int  getWaveNumber() const { return waveNumber; }

    // Getters pentru rendering
    // cppcheck-suppress unusedFunction
    const std::vector<std::unique_ptr<Tower>>& getTowers() const { return towers; }
    // cppcheck-suppress unusedFunction
    const Wave& getCurrentWave() const { return currentWave; }
    // cppcheck-suppress unusedFunction
    const std::vector<std::pair<int, int>>& getPath() const { return path; }
    // cppcheck-suppress unusedFunction
    int getPlayerHP() const { return playerHP; }
    // cppcheck-suppress unusedFunction
    int getMoney() const { return money; }

    // cppcheck-suppress unusedFunction
    const GlobalStatBuffs& getBuffs() const { return buffs_; }
    // cppcheck-suppress unusedFunction
    GlobalStatBuffs&         mutableBuffs() { return buffs_; }


    // ShopPanel reutilizeaza RNG-ul Game-ului pentru ca shop_state sa fie deterministic
    // cppcheck-suppress unusedFunction
    std::mt19937&  mutableRng() { return rng_; }


    // ShopPanel decrementeaza direct la cumparare (apare un check `money < cost` inainte).
    // cppcheck-suppress unusedFunction
    int&  mutableMoney() { return money; }

    // Statistici cumulative (resetate per run).
    // cppcheck-suppress unusedFunction
    int  getTotalKills()       const { return total_kills_; }
    // cppcheck-suppress unusedFunction
    int  getTotalMoneyEarned() const { return total_money_earned_; }
    // cppcheck-suppress unusedFunction
    int& mutableWeight()             { return player_weight_; }
    // cppcheck-suppress unusedFunction
    bool isEndlessActive()     const { return endless_active_; }
    // cppcheck-suppress unusedFunction
    void enableEndless()             { endless_active_ = true; }

    // cppcheck-suppress unusedFunction
    const std::string& getMapId() const { return current_map_id_; }

    // Save / Load

    void serializeTo(SaveData& out) const;
    void restoreFrom(const SaveData& src);
    // cppcheck-suppress unusedFunction
    int  getMaxWaves()   const { return max_waves_; }

    void placeTower(int typeChoice, int x, int y);

    // Tower-ul de la (col, row) sau nullptr. Versiunea non-const e folosita de
    // GameScene pentru actiuni pe turn (cycle targeting, apply token).
    Tower*       towerAt(int col, int row);
    const Tower* towerAt(int col, int row) const;

    // Repozitioneaza un turn cu abilitatea MOVABLE. Arunca InvalidPlacementException
    // daca turnul lipseste, nu e movable, sau celula destinatie e invalida.
    void moveTower(int fromCol, int fromRow, int toCol, int toRow);

    // Sell tower at (col, row): refund 75% cost base + 50% suma token-uri aplicate.
    // Returneaza 0 daca nu exista tower acolo.
    int  sellTower(int col, int row);

    void startWave();
    void tickWave(float dt);
    bool isWaveActive() const;
    void endWave();

    // Evenimentele vizuale ale ultimelor tick-uri (trageri/impacturi/morti),
    // consumate o data pe frame de EffectsLayer prin GameScene.
    std::vector<VisualEvent> takeVisualEvents() { return currentWave.takeVisualEvents(); }
    // Venitul per-Miner incasat la endWave (monedele de pe tabla).
    std::vector<IncomeEvent> takeIncomeEvents();

    bool isGameOver() const;
    bool allWavesDone() const;

    // Loterie / gauntlet
    bool lotteryOfferPending() const { return lottery_offer_pending_; }
    void acceptLottery();
    void declineLottery();
    bool isGauntletActive()  const { return gauntlet_waves_left_ > 0; }
    int  gauntletWavesLeft() const { return gauntlet_waves_left_; }
    bool isHardcoreRoute()   const { return lottery_accepted_; }
    // True O SINGURA data, imediat dupa ce gauntlet-ul s-a incheiat cu succes
    // (banii sunt deja adaugati; caller-ul acorda Mythic Token-ul).
    bool takeGauntletReward();

    friend std::ostream& operator<<(std::ostream& os, const Game& g);
};
