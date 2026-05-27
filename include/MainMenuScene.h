#pragma once
#include "Scene.h"
#include <SFML/Graphics.hpp>

class SceneManager;
class DataRegistry;

class MainMenuScene : public Scene {
    SceneManager&  manager_;
    const DataRegistry&  registry_;

    sf::Font  font_;
    bool has_font_ = false;

    sf::Text title_;
    sf::Text subtitle_;

    struct MenuButton {
        sf::RectangleShape rect;
        sf::Text           label;
        
        bool contains(float mx, float my) const {
            return rect.getGlobalBounds().contains(mx, my);
        }
    };
    MenuButton start_btn_;
    MenuButton continue_btn_;
    MenuButton quit_btn_;

    bool has_save_ = false;     

    void setupButton(MenuButton& btn, const char* text, float y, sf::Color fill);

public:
    MainMenuScene(SceneManager& manager, const DataRegistry& registry);

    void update(float dt) override;
    void render(sf::RenderWindow& window) override;
    void handleEvent(const sf::Event& event) override;
};
