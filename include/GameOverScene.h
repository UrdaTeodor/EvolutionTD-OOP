#pragma once
#include "Scene.h"
#include "HighScores.h"
#include <SFML/Graphics.hpp>
#include <string>

class SceneManager;
class DataRegistry;

// Construit din GameScene cu stats finale (wave reached, kills, money earned, victory flag).
// In ctor: scrie record nou in high_scores.json si sterge save_current.json (run terminat).

class GameOverScene : public Scene {
public:
    GameOverScene(SceneManager& manager, const DataRegistry& registry,
                  bool victory, const std::string& map_id,
                  int wave_reached, int kills, int money_earned);

    void update(float dt) override;
    void render(sf::RenderWindow& window) override;
    void handleEvent(const sf::Event& event) override;

private:
    SceneManager& manager_;
    const DataRegistry& registry_;
    bool victory_;
    std::string map_id_;
    int wave_reached_;
    int kills_;
    int money_earned_;
    HighScores::MapStats best_stats_;   // dupa update high_scores

    sf::Font font_;
    bool has_font_ = false;

    sf::Text title_;
    sf::Text stats_;
    sf::Text highscore_;

    struct MenuButton {
        sf::RectangleShape rect;
        sf::Text label;
        bool contains(float mx, float my) const {
            return rect.getGlobalBounds().contains(mx, my);
        }
    };
    MenuButton restart_btn_;
    MenuButton menu_btn_;
    MenuButton quit_btn_;

    void setupButton(MenuButton& btn, const char* text, float y, sf::Color fill);
};
