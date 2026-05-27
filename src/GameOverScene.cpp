#include "GameOverScene.h"
#include "SceneManager.h"
#include "MainMenuScene.h"
#include "GameScene.h"
#include "SaveManager.h"

namespace {
    constexpr float WIN_W = 1920.0f;
    constexpr float WIN_H = 1080.0f;
    constexpr float BTN_W = 380.0f;
    constexpr float BTN_H = 90.0f;
    constexpr float BTN_GAP = 30.0f;
    constexpr float BTN_Y = 760.0f;

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

GameOverScene::GameOverScene(SceneManager& manager, const DataRegistry& registry,
                             bool victory, const std::string& map_id,
                             int wave_reached, int kills, int money_earned)
    : manager_(manager), registry_(registry),
      victory_(victory), map_id_(map_id),
      wave_reached_(wave_reached), kills_(kills), money_earned_(money_earned) {

    // Record + cleanup save (run terminat, NU mai exista Continue).
    HighScores hs;
    hs.recordRun(map_id_, wave_reached_, kills_, money_earned_);
    best_stats_ = hs.getStats(map_id_);
    SaveManager::deleteSave();

    has_font_ = loadFont(font_);

    if (has_font_) {
        title_.setFont(font_);
        title_.setString(victory_ ? "VICTORY!" : "GAME OVER");
        title_.setCharacterSize(120);
        title_.setFillColor(victory_ ? sf::Color(120, 220, 140)
                                     : sf::Color(220, 100, 100));
        sf::FloatRect tb = title_.getLocalBounds();
        title_.setPosition((WIN_W - tb.width) / 2.0f, 200.0f);

        stats_.setFont(font_);
        stats_.setString(
            "Wave reached:  " + std::to_string(wave_reached_) + "\n"
            "Enemies killed: " + std::to_string(kills_) + "\n"
            "Money earned:  " + std::to_string(money_earned_)
        );
        stats_.setCharacterSize(40);
        stats_.setFillColor(sf::Color(220, 230, 255));
        sf::FloatRect sb = stats_.getLocalBounds();
        stats_.setPosition((WIN_W - sb.width) / 2.0f, 400.0f);

        highscore_.setFont(font_);
        highscore_.setString(
            "Best wave (all runs): " + std::to_string(best_stats_.best_wave_reached) +
            "    Total runs: " + std::to_string(best_stats_.total_runs)
        );
        highscore_.setCharacterSize(26);
        highscore_.setFillColor(sf::Color(180, 200, 230));
        sf::FloatRect hb = highscore_.getLocalBounds();
        highscore_.setPosition((WIN_W - hb.width) / 2.0f, 660.0f);
    }

    float totalBtnW = 3 * BTN_W + 2 * BTN_GAP;
    float startX    = (WIN_W - totalBtnW) / 2.0f;

    auto place = [&](MenuButton& b, int idx, const char* text, sf::Color fill) {
        float x = startX + idx * (BTN_W + BTN_GAP);
        b.rect.setSize(sf::Vector2f(BTN_W, BTN_H));
        b.rect.setPosition(x, BTN_Y);
        b.rect.setFillColor(fill);
        b.rect.setOutlineColor(sf::Color(220, 230, 255));
        b.rect.setOutlineThickness(2.0f);
        if (has_font_) {
            b.label.setFont(font_);
            b.label.setString(text);
            b.label.setCharacterSize(28);
            b.label.setFillColor(sf::Color::White);
            sf::FloatRect lb = b.label.getLocalBounds();
            b.label.setPosition(x + (BTN_W - lb.width) / 2.0f - lb.left,
                                BTN_Y + (BTN_H - lb.height) / 2.0f - lb.top);
        }
    };
    place(restart_btn_, 0, "Restart",   sf::Color( 60, 110,  80));
    place(menu_btn_,    1, "Main Menu", sf::Color( 80,  90, 130));
    place(quit_btn_,    2, "Quit Game", sf::Color(140,  80,  80));
}

void GameOverScene::setupButton(MenuButton& /*btn*/, const char* /*text*/, float /*y*/, sf::Color /*fill*/) {
    // Lasat declarat in .h pentru paritate cu PauseScene/MainMenuScene
}

void GameOverScene::update(float /*dt*/) {}

void GameOverScene::render(sf::RenderWindow& window) {
    window.clear(sf::Color(15, 18, 28));

    if (has_font_) {
        window.draw(title_);
        window.draw(stats_);
        window.draw(highscore_);
    }
    for (const auto* btn : { &restart_btn_, &menu_btn_, &quit_btn_ }) {
        window.draw(btn->rect);
        if (has_font_) window.draw(btn->label);
    }
}

void GameOverScene::handleEvent(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        manager_.requestReplace(std::make_unique<MainMenuScene>(manager_, registry_));
        return;
    }
    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left) {
        float mx = static_cast<float>(event.mouseButton.x);
        float my = static_cast<float>(event.mouseButton.y);

        if (restart_btn_.contains(mx, my)) {
            manager_.requestReplace(std::make_unique<GameScene>(manager_, registry_));
            return;
        }
        if (menu_btn_.contains(mx, my)) {
            manager_.requestReplace(std::make_unique<MainMenuScene>(manager_, registry_));
            return;
        }
        if (quit_btn_.contains(mx, my)) {
            manager_.requestClear();
            return;
        }
    }
}
