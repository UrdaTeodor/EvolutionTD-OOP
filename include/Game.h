#pragma once
#include <vector>
#include <ostream>
#include <utility>
#include <memory>     // pentru std::unique_ptr
#include "Tower.h"
#include "Wave.h"


// Contine grid-ul, turnurile, valul curent, HP-ul jucatorului, banii.
class Game {
    static constexpr int GRID_SIZE      = 20;
    static constexpr int MAX_WAVES      = 5;
    static constexpr int STARTING_HP    = 100;
    static constexpr int STARTING_MONEY = 105;

    char grid[GRID_SIZE][GRID_SIZE];     // display grid: '.', 'P', 'A', 'D', 'H', 'F', 'E', 'M'
    bool pathGrid[GRID_SIZE][GRID_SIZE]; // true = celula e pe drum

    // vector de pointeri la baza Tower.
    // unique_ptr<Tower> = Game detine turnurile, le sterge automat in destructor.
    std::vector<std::unique_ptr<Tower>> towers;

    Wave currentWave;
    std::vector<std::pair<int, int>> path;  // waypoints: (start finish curbe)
    int playerHP;
    int money;
    int waveNumber;

    
    // Snapshot: o copie a  la inceputul ultimului val (luata in runWave()).
    // Permite undo restoreSnapshot()
    std::unique_ptr<Game> snapshot_;

    void initPath();                     // construieste path-ul si pathGrid
    void refreshGrid();                  // reconstruieste grid-ul de afisat
    bool isPathCell(int x, int y) const;
    bool isOccupied(int x, int y) const;

    // static = nu depinde de instanta de Game
    static Wave buildWave(int waveNum);


    // needsPath = true daca turnul cere sa fie pe drum (Firewall).
    bool isValidPlacement(int x, int y, bool needsPath) const;

public:
    Game();  

    //Regula celor 3 + copy-and-swap (cerinta T2)
    // Game detine vector<unique_ptr<Tower>>, deci cc/op= generate ar fi sterse.
    // manual -> copy polimorfic prin Tower::clone().
    Game(const Game& other);
    Game& operator=(Game other);                 // by-value -> copy+swap
    ~Game();                                     // out of line pt unique_ptr<self>
    friend void swap(Game& a, Game& b) noexcept;

    void takeSnapshot();                         // apelat automat la inceput de val
    bool restoreSnapshot();                      // returneaza false daca nu exista snapshot
    int  getWaveNumber() const { return waveNumber; }   // pt sincronizare cu main

    // Getters pentru rendering 
    const std::vector<std::unique_ptr<Tower>>& getTowers()      const { return towers; }
    const Wave&                                getCurrentWave() const { return currentWave; }
    const std::vector<std::pair<int, int>>&    getPath()        const { return path; }
    int                                        getPlayerHP()    const { return playerHP; }
    int                                        getMoney()       const { return money; }
    // cppcheck-suppress unusedFunction // public API expus pentru renderer / T3
    static constexpr int                       getGridSize()          { return GRID_SIZE; }
    static constexpr int                       getMaxWaves()          { return MAX_WAVES; }

    // typeChoice: 1=Antivirus, 2=Adblocker, 3=Honeypot, 4=Firewall
    // Arunca InvalidPlacementException sau InsufficientFundsException la eroare.
    // Apelantul din main are try/catch.
    void placeTower(int typeChoice, int x, int y);

    void runWave();

    // Frame-by-frame API
    // Ordinea de apel: startWave() -> tickWave(dt) cat timp isWaveActive() -> endWave().
    void startWave();
    void tickWave(float dt);
    bool isWaveActive() const;
    void endWave();

    void displayGrid() const;
    bool isGameOver() const;
    bool allWavesDone() const;

    friend std::ostream& operator<<(std::ostream& os, const Game& g);
};
