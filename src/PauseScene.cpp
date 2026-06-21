#include "PauseScene.h"
#include "SceneManager.h"
#include "GameScene.h"
#include "MainMenuScene.h"

namespace {
    constexpr float WIN_W   = 1920.0f;
    constexpr float WIN_H   = 1080.0f;
    constexpr float BTN_W   = 420.0f;
    constexpr float BTN_H   = 90.0f;
    constexpr float BTN_X   = (WIN_W - BTN_W) / 2.0f;
    constexpr float BTN_GAP = 30.0f;
    constexpr float BTN_Y_START = 460.0f;

    bool loadFont(sf::Font& font) {
        const char* paths[] = {
            "assets/font.ttf",                  // bundled (portabil, cautat primul)
            "C:/Windows/Fonts/segoeui.ttf",     // Windows
            "C:/Windows/Fonts/arial.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",         // Debian/Ubuntu
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/dejavu/DejaVuSans.ttf",                  // Fedora
            "/usr/share/fonts/TTF/DejaVuSans.ttf",                     // Arch
            "/System/Library/Fonts/Supplemental/Arial.ttf",           // macOS
            "/Library/Fonts/Arial.ttf",
        };
        for (const char* p : paths) {
            if (font.loadFromFile(p)) return true;
        }
        return false;
    }
}

PauseScene::PauseScene(SceneManager& manager, GameScene& game_scene, const DataRegistry& registry)
    : manager_(manager), game_scene_(game_scene), registry_(registry) {
    has_font_ = loadFont(font_);

    if (has_font_) {
        title_.setFont(font_);
        title_.setString("PAUSED");
        title_.setCharacterSize(96);
        title_.setFillColor(sf::Color(220, 230, 255));
        sf::FloatRect tb = title_.getLocalBounds();
        title_.setPosition((WIN_W - tb.width) / 2.0f, 280.0f);
    }

    setupButton(resume_btn_,    "Resume (ESC)",      BTN_Y_START,                       sf::Color( 60, 110,  80));
    setupButton(save_menu_btn_, "Save & Main Menu",  BTN_Y_START + (BTN_H + BTN_GAP) * 1.0f, sf::Color( 80,  90, 130));
    setupButton(quit_btn_,      "Save & Quit",       BTN_Y_START + (BTN_H + BTN_GAP) * 2.0f, sf::Color(140,  80,  80));
}

void PauseScene::setupButton(MenuButton& btn, const char* text, float y, sf::Color fill) {
    btn.rect.setSize(sf::Vector2f(BTN_W, BTN_H));
    btn.rect.setPosition(BTN_X, y);
    btn.rect.setFillColor(fill);
    btn.rect.setOutlineColor(sf::Color(220, 230, 255));
    btn.rect.setOutlineThickness(2.0f);

    if (has_font_) {
        btn.label.setFont(font_);
        btn.label.setString(text);
        btn.label.setCharacterSize(32);
        btn.label.setFillColor(sf::Color::White);
        sf::FloatRect lb = btn.label.getLocalBounds();
        btn.label.setPosition(BTN_X + (BTN_W - lb.width) / 2.0f - lb.left,
                              y + (BTN_H - lb.height) / 2.0f - lb.top);
    }
}

void PauseScene::update(float /*dt*/) {
    // static
}

void PauseScene::render(sf::RenderWindow& window) {
    // Dim overlay peste fundal (GameScene a desenat deja).
    sf::RectangleShape dim(sf::Vector2f(WIN_W, WIN_H));
    dim.setFillColor(sf::Color(0, 0, 0, 170));
    window.draw(dim);

    if (has_font_) window.draw(title_);

    for (const auto* btn : { &resume_btn_, &save_menu_btn_, &quit_btn_ }) {
        window.draw(btn->rect);
        if (has_font_) window.draw(btn->label);
    }
}

void PauseScene::handleEvent(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        manager_.requestPop();   // Resume
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left) {
        float mx = static_cast<float>(event.mouseButton.x);
        float my = static_cast<float>(event.mouseButton.y);

        if (resume_btn_.contains(mx, my)) {
            manager_.requestPop();
            return;
        }
        if (save_menu_btn_.contains(mx, my)) {
            game_scene_.saveNow();
            manager_.requestReplace(std::make_unique<MainMenuScene>(manager_, registry_));
            return;
        }
        if (quit_btn_.contains(mx, my)) {
            game_scene_.saveNow();
            manager_.requestClear();
            return;
        }
    }
}
