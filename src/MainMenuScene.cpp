#include "MainMenuScene.h"
#include "SceneManager.h"
#include "GameScene.h"
#include "SaveManager.h"
#include "SaveData.h"
#include "GameException.h"
#include <iostream>

namespace {
    constexpr float WIN_W      = 1920.0f;
    constexpr float BTN_W      = 420.0f;
    constexpr float BTN_H      = 96.0f;
    constexpr float BTN_X      = (WIN_W - BTN_W) / 2.0f;
    constexpr float BTN_GAP    = 40.0f;
    constexpr float BTN_Y_START = 480.0f;

    bool loadFont(sf::Font& font) {
        const char* paths[] = {
            "assets/font.ttf",
            "C:/Windows/Fonts/segoeui.ttf",
            "C:/Windows/Fonts/arial.ttf",
        };
        for (const char* p : paths) {
            if (font.loadFromFile(p)) return true;
        }
        return false;
    }
}

MainMenuScene::MainMenuScene(SceneManager& manager, const DataRegistry& registry)
    : manager_(manager), registry_(registry) {
    has_font_ = loadFont(font_);

    if (has_font_) {
        title_.setFont(font_);
        title_.setString("EvolutionTD");
        title_.setCharacterSize(96);
        title_.setFillColor(sf::Color(220, 230, 255));
        sf::FloatRect tb = title_.getLocalBounds();
        title_.setPosition((WIN_W - tb.width) / 2.0f, 220.0f);

        subtitle_.setFont(font_);
        subtitle_.setCharacterSize(36);
        subtitle_.setFillColor(sf::Color(150, 180, 210));
        sf::FloatRect sb = subtitle_.getLocalBounds();
        subtitle_.setPosition((WIN_W - sb.width) / 2.0f, 340.0f);
    }

    has_save_ = SaveManager::saveExists();

    setupButton(start_btn_,    "Start",    BTN_Y_START - 50,                            sf::Color( 80, 140, 200));
    setupButton(continue_btn_, "Continue", BTN_Y_START + (BTN_H + BTN_GAP) * 1.0f - 50,
                has_save_ ? sf::Color( 80, 180, 120) : sf::Color( 60,  60,  60));
    setupButton(quit_btn_,     "Quit",     BTN_Y_START + (BTN_H + BTN_GAP) * 2.0f - 50, sf::Color(140,  80,  80));
}

void MainMenuScene::setupButton(MenuButton& btn, const char* text, float y, sf::Color fill) {
    btn.rect.setSize(sf::Vector2f(BTN_W, BTN_H));
    btn.rect.setPosition(BTN_X, y);
    btn.rect.setFillColor(fill);
    btn.rect.setOutlineColor(sf::Color(220, 230, 255));
    btn.rect.setOutlineThickness(2.0f);

    if (has_font_) {
        btn.label.setFont(font_);
        btn.label.setString(text);
        btn.label.setCharacterSize(40);
        btn.label.setFillColor(sf::Color::White);
        sf::FloatRect lb = btn.label.getLocalBounds();
        btn.label.setPosition(BTN_X + (BTN_W - lb.width) / 2.0f - lb.left,
                              y + (BTN_H - lb.height) / 2.0f - lb.top);
    }
}

void MainMenuScene::update(float /*dt*/) {
    // Statice momentan
}

void MainMenuScene::render(sf::RenderWindow& window) {
    window.clear(sf::Color(15, 18, 28));

    if (has_font_) {
        window.draw(title_);
        window.draw(subtitle_);
        window.draw(start_btn_.rect);
        window.draw(start_btn_.label);
        window.draw(continue_btn_.rect);
        window.draw(continue_btn_.label);
        window.draw(quit_btn_.rect);
        window.draw(quit_btn_.label);
    } else {
        // Fallback dreptunghiuri coloratee.
        window.draw(start_btn_.rect);
        window.draw(continue_btn_.rect);
        window.draw(quit_btn_.rect);
    }
}

void MainMenuScene::handleEvent(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        manager_.requestClear();
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left) {
        float mx = static_cast<float>(event.mouseButton.x);
        float my = static_cast<float>(event.mouseButton.y);

        if (start_btn_.contains(mx, my)) {
            // Start nou daca exista save vechi, e overwrited la prima actiune in GameScene.
            manager_.requestReplace(std::make_unique<GameScene>(manager_, registry_));
        } else if (continue_btn_.contains(mx, my) && has_save_) {
            try {
                SaveData d = SaveManager::read();
                manager_.requestReplace(std::make_unique<GameScene>(manager_, registry_, d));
            } catch (const GameException& err) {
                std::cerr << "Continue failed: " << err.what() << "\n";
                // Daca save e corupt, fallback la run nou.
                manager_.requestReplace(std::make_unique<GameScene>(manager_, registry_));
            }
        } else if (quit_btn_.contains(mx, my)) {
            manager_.requestClear();
        }
    }
}
