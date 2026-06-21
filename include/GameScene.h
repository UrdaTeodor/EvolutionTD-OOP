#pragma once
#include "Scene.h"
#include "Game.h"
#include "ShopPanel.h"
#include "Sprites.h"
#include "BoardRenderer.h"
#include "EffectsLayer.h"
#include "HudPanel.h"
#include <SFML/Graphics.hpp>
#include <random>
#include <string>

class SceneManager;
class DataRegistry;
struct SaveData;

// Dirijorul scenei de joc: detine Game-ul (logica) si ShopPanel-ul, ruteaza
// input-ul si deleaga randarea catre BoardRenderer / EffectsLayer / HudPanel.
class GameScene : public Scene {
public:
    GameScene(SceneManager& manager, const DataRegistry& registry);
    // Ctor de Continue: porneste cu state restaurat din save_current.json.
    GameScene(SceneManager& manager, const DataRegistry& registry, const SaveData& save);

    void update(float dt) override;
    void render(sf::RenderWindow& window) override;
    void handleEvent(const sf::Event& event) override;

    // Trigger save (apelat de PauseScene Save & Quit / Save & Main Menu).
    void saveNow();

private:
    SceneManager&        manager_;
    const DataRegistry&  registry_;

    sf::Font font_;
    bool     has_font_;
    Sprites  sprites_;

    Game          game_;
    BoardRenderer board_;
    EffectsLayer  effects_;
    HudPanel      hud_;
    ShopPanel     shop_;

    bool wave_running_   = false;
    int  selected_type_  = 0;
    bool boss_was_active_ = false;
    int  game_speed_     = 0;     // 0 = intre valuri, 1/2/4 = multiplicator
    bool token_apply_mode_ = false;
    bool move_mode_        = false;   // turn selectat cu MOVABLE: urmatorul click pe grid il muta
    bool game_over_triggered_ = false;

    int selected_tower_col_ = -1;
    int selected_tower_row_ = -1;

    std::string last_error_;
    sf::Clock   err_clock_;

    // Screen shake + vignette la pierdere de HP. ui_rng_ e doar pentru efecte
    // vizuale, separat de RNG-ul de gameplay din Game (seedabil/salvabil).
    int          prev_hp_     = 0;
    float        shake_timer_ = 0.0f;
    float        leak_flash_  = 0.0f;   // vignette rosie: leak-ul DOARE vizibil
    std::mt19937 ui_rng_{std::random_device{}()};

    void cycleGameSpeed();
    void refreshShopForCurrentWave();
    void checkGameOver();

    // Apply token activ pe tower-ul de la (col,row).
    //  APPLIED  = token consumat;
    //  REJECTED = turn valid dar evolutia a fost refuzata cu motiv (mesajul e
    //             deja setat — NU il suprascrie cu "Cancelled");
    //  NO_TARGET = click pe celula fara turn compatibil.
    enum class TokenApply { APPLIED, REJECTED, NO_TARGET };
    TokenApply tryApplyActiveTokenOnTower(int col, int row);

    const Tower* findTowerAt(int col, int row) const;
    void sellSelectedTower();
    void cycleSelectedTowerTargeting();
    void enterMoveMode();

    // Mesaj pe HUD (jos-stanga) care expira dupa 3 secunde.
    void setMessage(const std::string& msg);

    std::string buildStatus() const;
    void onLeftClick(float mx, float my);
};
