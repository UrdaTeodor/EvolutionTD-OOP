#pragma once
#include <vector>
#include <ostream>
#include <utility>
#include "Tower.h"
#include "Wave.h"

class Game {
    static constexpr int GRID_SIZE    = 20;
    static constexpr int MAX_WAVES    = 3;
    static constexpr int STARTING_HP  = 100;
    static constexpr int STARTING_MONEY = 105;

    char grid[GRID_SIZE][GRID_SIZE];     // display grid: '.', 'P', 'A', 'D', 'H', 'F', 'E'
    bool pathGrid[GRID_SIZE][GRID_SIZE];
    std::vector<Tower> towers;
    Wave currentWave;
    std::vector<std::pair<int, int>> path; // waypoints (start finish curbe)
    int playerHP;
    int money;
    int waveNumber;

    void initPath();
    void refreshGrid();
    bool isPathCell(int x, int y) const;
    bool isOccupied(int x, int y) const;
    static Wave buildWave(int waveNum);
    bool isValidPlacement(int x, int y, TowerType type) const;

public:
    Game();

    bool placeTower(int typeChoice, int x, int y);

    void runWave();       
    void displayGrid() const;
    bool isGameOver() const;
    bool allWavesDone() const;

    friend std::ostream& operator<<(std::ostream& os, const Game& g);
};
