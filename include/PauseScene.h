#pragma once
#include "Scene.h"
#include <SFML/Graphics.hpp>

class SceneManager;
class GameScene;
class DataRegistry;


// 3 butoane:
//   Resume -> manager.pop(), GameScene revine activ
//   Save & Main Menu -> game_scene_->saveNow(); manager.replace(MainMenuScene)
//   Quit Game -> game_scene_->saveNow(); manager.clear() -> main loop iese
class PauseScene : public Scene {
public:
    PauseScene(SceneManager& manager, GameScene& game_scene, const DataRegistry& registry);

    void update(float dt) override;
    void render(sf::RenderWindow& window) override;
    void handleEvent(const sf::Event& event) override;

private:
    SceneManager&  manager_;
    GameScene& game_scene_;
    const DataRegistry& registry_;

    sf::Font font_;
    bool has_font_ = false;

    sf::Text title_;
    struct MenuButton {
        sf::RectangleShape rect;
        sf::Text label;
        bool contains(float mx, float my) const {
            return rect.getGlobalBounds().contains(mx, my);
        }
    };
    MenuButton resume_btn_;
    MenuButton save_menu_btn_;
    MenuButton quit_btn_;

    void setupButton(MenuButton& btn, const char* text, float y, sf::Color fill);
};
