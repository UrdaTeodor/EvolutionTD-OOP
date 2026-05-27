#pragma once
#include "Scene.h"
#include "Game.h"
#include "Enemy.h"
#include "ShopPanel.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class SceneManager;
class DataRegistry;
struct SaveData;

// Tine instanta Game-ului, sprite-urile, butoanele de tower si starea de val.
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
    static constexpr unsigned WIN_W   = 1920;
    static constexpr unsigned WIN_H   = 1080;
    static constexpr unsigned GRID_PX = 1080;
    static constexpr unsigned GRID_X  = (WIN_W - GRID_PX) / 2;
    static constexpr int      CELLS   = 20;
    static constexpr float    CELL_PX = static_cast<float>(GRID_PX) / CELLS;

    //structuri interne

    // Sprite-uri tower (5) + enemy (4) + boss phase-uri (5). Fallback la cercuri.
    struct Sprites {
        sf::Texture towerTex[5];
        bool        towerOk[5] = {false, false, false, false, false};
        sf::Texture enemyTex[4];
        bool        enemyOk[4] = {false, false, false, false};
        sf::Texture bossPhases[5];
        bool        bossPhaseOk[5] = {false, false, false, false, false};
        sf::Texture fireTrailTex;
        bool        fireTrailOk = false;

        void load();

        static int towerIndex(char c);
        static int enemyIndex(const std::string& name);
        static float displayScale(const std::string& name);
        static int bossPhaseForRatio(float ratio);

        const sf::Texture* textureFor(const Enemy& enemy) const;
    };

    struct Button {
        sf::RectangleShape rect;
        sf::Text label;
        int actionId = 0;
        bool contains(float mx, float my) const {
            return rect.getGlobalBounds().contains(mx, my);
        }
    };

    struct Projectile {
        sf::Vector2f start;
        sf::Vector2f end;
        float progress;
        float duration;
        sf::Color color;
        bool has_fire_trail = false;
        float trail_spawn_timer = 0.0f;
    };

    // Particula vizuala lasata de proiectilele cu FIRE_TRAIL
    struct TrailParticle {
        sf::Vector2f pos;
        float        age;
        float        lifetime;
    };

    SceneManager&        manager_;
    const DataRegistry&  registry_;

    //resurse SFML
    sf::Font font_;
    bool has_font_ = false;
    Sprites sprites_;
    sf::VertexArray grid_quads_;
    sf::VertexArray grid_lines_;
    sf::RectangleShape ui_left_;
    sf::RectangleShape ui_right_;
    std::vector<Button> tower_buttons_;
    Button ff_button_;
    Button shop_button_;
    sf::Text hp_text_;
    sf::Text money_text_;
    sf::Text wave_text_;
    sf::Text status_text_;
    sf::Text last_err_text_;


    Game game_;
    bool wave_running_ = false;
    int selected_type_ = 0;
    std::string  last_error_;
    sf::Clock err_clock_;
    sf::Clock game_clock_;

    std::vector<float>         last_fire_time_;
    std::vector<Projectile>    projectiles_;
    std::vector<TrailParticle> trail_particles_;

    bool boss_was_active_ = false;

    // Fast-forward 
    int game_speed_ = 0;

    // Shop 
    ShopPanel shop_;
    bool token_apply_mode_ = false;  

    // GameOver / Victory trigger o singura data prin SceneManager
    bool                 game_over_triggered_ = false;

    int                  selected_tower_col_ = -1;
    int                  selected_tower_row_ = -1;

    // helpers init
    void initFont();
    void initSpritesAndPanels();
    void initGridQuads();
    void initButtons();
    void initText();

    //helpers update/render
    sf::Vector2f cellToPx(int row, int col) const;
    sf::Vector2f cellCenter(float colF, float rowF) const;
    void cycleGameSpeed();
    void updateFFLabel();
    void updateShopButtonLabel();
    void refreshShopForCurrentWave();
    void tickProjectileFiring(float dt);
    void drawRangeCircle(sf::RenderWindow& window, sf::Vector2f center, float radiusPx,
                         sf::Color fill, sf::Color outline) const;

    // Apply token activ pe tower-ul de la (col,row) daca e compatibil; intoarce true daca consumat.
    bool tryApplyActiveTokenOnTower(int col, int row);

    void checkGameOver();

    const Tower* findTowerAt(int col, int row) const;
    void sellSelectedTower();
    void renderTowerInfoPanel(sf::RenderWindow& window, const Tower* tower);
};
