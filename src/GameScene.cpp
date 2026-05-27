#include "GameScene.h"
#include "SceneManager.h"
#include "MainMenuScene.h"
#include "PauseScene.h"
#include "GameOverScene.h"
#include "SaveManager.h"
#include "SaveData.h"
#include "DataRegistry.h"
#include "GameException.h"
#include "GlobalStatBuffs.h"
#include "Tower.h"
#include "AbilityType.h"
#include "AbilityEvolution.h"
#include "EvolutionContext.h"
#include "WaveSpec.h"
#include "MapSpec.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <utility>

// =====================================================================
// Helpers locale
// =====================================================================
namespace {

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

    sf::Color towerColor(char c) {
        switch (c) {
            case 'A': return sf::Color( 80, 140, 230);
            case 'D': return sf::Color(230, 210,  60);
            case 'H': return sf::Color(230, 150,  60);
            case 'F': return sf::Color(180,  80, 200);
            case 'M': return sf::Color(220, 180,  60);
            default:  return sf::Color::White;
        }
    }

    sf::Color rarityToColor(Evolution::Rarity r) {
        switch (r) {
            case Evolution::Rarity::MINI:      return sf::Color(140, 180, 200);
            case Evolution::Rarity::RARE:      return sf::Color( 60, 140, 220);
            case Evolution::Rarity::EPIC:      return sf::Color(160,  80, 200);
            case Evolution::Rarity::LEGENDARY: return sf::Color(220, 140,  40);
            case Evolution::Rarity::MYTHIC:    return sf::Color(220,  60,  60);
        }
        return sf::Color::White;
    }

    sf::FloatRect tokenSlotRect(int i) {
        return { 20.0f + i * 65.0f, 520.0f, 50.0f, 50.0f };
    }

    float rangeForType(int type) {
        switch (type) {
            case 1: return 4.0f;
            case 2: return 3.0f;
            case 3: return 1.5f;
            default: return 0.0f;
        }
    }

    float fireIntervalForChar(char c) {
        switch (c) {
            case 'A': return 1.0f;
            case 'D': return 0.25f;
            default:  return 0.0f;
        }
    }

    float projectileSpeedForChar(char c) {
        switch (c) {
            case 'A': return 8.0f;
            case 'D': return 10.0f;
            default:  return 0.0f;
        }
    }

    // Acelasi calcul ca Tower::calculateInterceptPoint, dar pe coordonate float locale
    // ca proiectilele vizuale sa nimereasca acolo unde Tower::update aplica damage.
    std::pair<float, float> calculateIntercept(int towerCol, int towerRow,
                                               float ex, float ey,
                                               float vx, float vy,
                                               float projectileSpeed) {
        float dx = ex - static_cast<float>(towerCol);
        float dy = ey - static_cast<float>(towerRow);
        float ps = projectileSpeed;

        float a = vx * vx + vy * vy - ps * ps;
        float b = 2.0f * (dx * vx + dy * vy);
        float c = dx * dx + dy * dy;

        float t = 0.0f;
        if (std::abs(a) < 0.0001f) {
            t = (std::abs(b) > 0.0001f) ? (-c / b) : 0.0f;
        } else {
            float disc = b * b - 4.0f * a * c;
            if (disc < 0.0f) return {ex, ey};
            float sqrtDisc = std::sqrt(disc);
            float t1 = (-b + sqrtDisc) / (2.0f * a);
            float t2 = (-b - sqrtDisc) / (2.0f * a);
            if      (t1 > 0.0f && t2 > 0.0f) t = std::min(t1, t2);
            else if (t1 > 0.0f)              t = t1;
            else if (t2 > 0.0f)              t = t2;
            else return {ex, ey};
        }
        return {ex + vx * t, ey + vy * t};
    }

    std::vector<std::pair<int,int>> pathCells(const std::vector<std::pair<int,int>>& waypoints) {
        std::vector<std::pair<int,int>> cells;
        for (size_t i = 0; i + 1 < waypoints.size(); i++) {
            int r1 = waypoints[i].first,   c1 = waypoints[i].second;
            int r2 = waypoints[i+1].first, c2 = waypoints[i+1].second;
            if (r1 == r2) {
                for (int c = std::min(c1, c2); c <= std::max(c1, c2); c++)
                    cells.push_back({r1, c});
            } else {
                for (int r = std::min(r1, r2); r <= std::max(r1, r2); r++)
                    cells.push_back({r, c1});
            }
        }
        return cells;
    }

} // namespace

// GameScene::Sprites


void GameScene::Sprites::load() {
    const char* towerFiles[5] = {
        "assets/sprites/tower_antivirus.png",
        "assets/sprites/tower_adblocker.png",
        "assets/sprites/tower_honeypot.png",
        "assets/sprites/tower_firewall.png",
        "assets/sprites/tower_miner.png",
    };
    const char* enemyFiles[4] = {
        "assets/sprites/enemy_adware.png",
        "assets/sprites/enemy_trojan.png",
        "assets/sprites/enemy_worm.png",
        "assets/sprites/enemy_iloveyou.png",
    };
    const char* bossFiles[5] = {
        "assets/sprites/enemy_iloveyou_p1.png",
        "assets/sprites/enemy_iloveyou_p2.png",
        "assets/sprites/enemy_iloveyou_p3.png",
        "assets/sprites/enemy_iloveyou_p4.png",
        "assets/sprites/enemy_iloveyou_p5.png",
    };
    for (int i = 0; i < 5; i++) towerOk[i]     = towerTex[i].loadFromFile(towerFiles[i]);
    for (int i = 0; i < 4; i++) enemyOk[i]     = enemyTex[i].loadFromFile(enemyFiles[i]);
    for (int i = 0; i < 5; i++) bossPhaseOk[i] = bossPhases[i].loadFromFile(bossFiles[i]);
    fireTrailOk = fireTrailTex.loadFromFile("assets/sprites/fire_trail.png");
}

int GameScene::Sprites::towerIndex(char c) {
    switch (c) {
        case 'A': return 0;
        case 'D': return 1;
        case 'H': return 2;
        case 'F': return 3;
        case 'M': return 4;
        default:  return -1;
    }
}

int GameScene::Sprites::enemyIndex(const std::string& name) {
    if (name == "Adware")   return 0;
    if (name == "Trojan")   return 1;
    if (name == "Worm")     return 2;
    if (name == "ILOVEYOU") return 3;
    return -1;
}

float GameScene::Sprites::displayScale(const std::string& name) {
    if (name == "ILOVEYOU") return 2.0f;
    if (name == "Trojan")   return 1.4f;
    if (name == "Worm")     return 0.8f;
    return 1.0f;
}

int GameScene::Sprites::bossPhaseForRatio(float ratio) {
    if (ratio > 0.8f) return 0;
    if (ratio > 0.6f) return 1;
    if (ratio > 0.4f) return 2;
    if (ratio > 0.2f) return 3;
    return 4;
}

const sf::Texture* GameScene::Sprites::textureFor(const Enemy& enemy) const {
    const std::string& name = enemy.getName();
    if (name == "ILOVEYOU") {
        float ratio = (enemy.getMaxHealth() > 0.0f)
                          ? (enemy.getCurrentHealth() / enemy.getMaxHealth())
                          : 0.0f;
        int phase = bossPhaseForRatio(ratio);
        if (bossPhaseOk[phase]) return &bossPhases[phase];
        if (enemyOk[3])         return &enemyTex[3];
        return nullptr; //5 health stages
    }
    int idx = enemyIndex(name);
    if (idx >= 0 && enemyOk[idx]) return &enemyTex[idx];
    return nullptr;
}


// GameScene


GameScene::GameScene(SceneManager& manager, const DataRegistry& registry)
    : manager_(manager),
      registry_(registry),
      grid_quads_(sf::Quads),
      grid_lines_(sf::Lines),
      game_(registry),
      shop_(game_.mutableBuffs(), registry) {
    initFont();
    initSpritesAndPanels();
    initGridQuads();
    initButtons();
    initText();

    // Shop: incarca factories + font + offer initial pentru wave 1.
    shop_.loadFromJson("data/evolutions.json");
    if (has_font_) shop_.setFont(font_);
    refreshShopForCurrentWave();
    updateShopButtonLabel();
}

GameScene::GameScene(SceneManager& manager, const DataRegistry& registry, const SaveData& save)
    : GameScene(manager, registry) {
    // Restore peste state-ul fresh creat de ctor-ul de mai sus.
    game_.restoreFrom(save);
    shop_.restoreFrom(save);
    updateShopButtonLabel();
}

void GameScene::initFont() {
    has_font_ = loadFont(font_);
    if (!has_font_) std::cerr << "GameScene: nu am gasit niciun font.\n";
}

void GameScene::initSpritesAndPanels() {
    sprites_.load();

    ui_left_.setSize(sf::Vector2f(GRID_X, WIN_H));
    ui_left_.setFillColor(sf::Color(40, 40, 55));

    ui_right_.setSize(sf::Vector2f(GRID_X, WIN_H));
    ui_right_.setPosition(GRID_X + GRID_PX, 0);
    ui_right_.setFillColor(sf::Color(40, 40, 55));
}
//cu AI aici pentru gradientu asta.
void GameScene::initGridQuads() {
    auto cells = pathCells(game_.getPath());

    bool isOnPath[CELLS][CELLS] = {};
    for (const auto& [r, c] : cells) isOnPath[r][c] = true;

    std::mt19937 rng(0xC0DEBABE);
    auto vary = [&rng](sf::Color base, int amount) {
        std::uniform_int_distribution<int> d(-amount, amount);
        int delta = d(rng);
        int r = std::clamp(static_cast<int>(base.r) + delta, 0, 255);
        int g = std::clamp(static_cast<int>(base.g) + delta, 0, 255);
        int b = std::clamp(static_cast<int>(base.b) + delta, 0, 255);
        return sf::Color(static_cast<sf::Uint8>(r),
                         static_cast<sf::Uint8>(g),
                         static_cast<sf::Uint8>(b));
    };
    const sf::Color pathBase(28, 30, 36);
    const sf::Color bgBase  (108, 112, 120);
    for (int r = 0; r < CELLS; ++r) {
        for (int c = 0; c < CELLS; ++c) {
            sf::Color base = isOnPath[r][c] ? pathBase : bgBase;
            int amp        = isOnPath[r][c] ? 8 : 12;
            float x = GRID_X + c * CELL_PX;
            float y = r * CELL_PX;
            grid_quads_.append(sf::Vertex({x,           y},           vary(base, amp)));
            grid_quads_.append(sf::Vertex({x + CELL_PX, y},           vary(base, amp)));
            grid_quads_.append(sf::Vertex({x + CELL_PX, y + CELL_PX}, vary(base, amp)));
            grid_quads_.append(sf::Vertex({x,           y + CELL_PX}, vary(base, amp)));
        }
    }

    for (int i = 0; i <= CELLS; ++i) {
        grid_lines_.append(sf::Vertex(sf::Vector2f(GRID_X + i * CELL_PX, 0),       sf::Color(80, 200, 220, 25)));
        grid_lines_.append(sf::Vertex(sf::Vector2f(GRID_X + i * CELL_PX, GRID_PX), sf::Color(80, 200, 220, 25)));
        grid_lines_.append(sf::Vertex(sf::Vector2f(GRID_X,           i * CELL_PX), sf::Color(80, 200, 220, 25)));
        grid_lines_.append(sf::Vertex(sf::Vector2f(GRID_X + GRID_PX, i * CELL_PX), sf::Color(80, 200, 220, 25)));
    }
}

void GameScene::initButtons() {
    auto makeTowerButton = [this](int type, const char* text, float y) {
        Button b;
        b.actionId = type;
        b.rect.setSize(sf::Vector2f(380.0f, 90.0f));
        b.rect.setPosition(GRID_X + GRID_PX + 20.0f, y);
        b.rect.setFillColor(towerColor("ADHFM"[type - 1]));
        b.rect.setOutlineColor(sf::Color::Black);
        b.rect.setOutlineThickness(2.0f);
        if (has_font_) {
            b.label.setFont(font_);
            b.label.setString(text);
            b.label.setCharacterSize(22);
            b.label.setFillColor(sf::Color::Black);
            b.label.setPosition(GRID_X + GRID_PX + 40.0f, y + 28.0f);
        }
        return b;
    };

    if (has_font_) {
        tower_buttons_.push_back(makeTowerButton(1, " Antivirus  $50",       60.0f));
        tower_buttons_.push_back(makeTowerButton(2, " Adblocker  $40",      170.0f));
        tower_buttons_.push_back(makeTowerButton(3, " Honeypot   $30",      280.0f));
        tower_buttons_.push_back(makeTowerButton(4, " Firewall   $60",      390.0f));
        tower_buttons_.push_back(makeTowerButton(5, " Miner      $50",      500.0f));
    }

    // FF button: 0 = "START", click cicleaza 0 -> 1 -> 2 -> 4 -> 1. Reset la 0 la wave end.
    ff_button_.actionId = 99;
    ff_button_.rect.setSize(sf::Vector2f(380.0f, 80.0f));
    ff_button_.rect.setPosition(GRID_X + GRID_PX + 20.0f, 900.0f);
    ff_button_.rect.setFillColor(sf::Color(60, 110, 80));
    ff_button_.rect.setOutlineColor(sf::Color::Black);
    ff_button_.rect.setOutlineThickness(2.0f);
    if (has_font_) {
        ff_button_.label.setFont(font_);
        ff_button_.label.setCharacterSize(36);
        ff_button_.label.setFillColor(sf::Color::White);
        ff_button_.label.setPosition(GRID_X + GRID_PX + 40.0f, 920.0f);
    }
    updateFFLabel();   // setteaza textul corespunzator game_speed_ initial (0 -> "START")

    // Shop button 
    shop_button_.actionId = 100;
    shop_button_.rect.setSize(sf::Vector2f(380.0f, 80.0f));
    shop_button_.rect.setPosition(20.0f, 420.0f);
    shop_button_.rect.setFillColor(sf::Color(80, 60, 130));
    shop_button_.rect.setOutlineColor(sf::Color::Black);
    shop_button_.rect.setOutlineThickness(2.0f);
    if (has_font_) {
        shop_button_.label.setFont(font_);
        shop_button_.label.setString("OPEN SHOP");
        shop_button_.label.setCharacterSize(26);
        shop_button_.label.setFillColor(sf::Color::White);
        shop_button_.label.setPosition(50.0f, 440.0f);
    }
}

void GameScene::initText() {
    if (!has_font_) return;
    auto setupText = [&](sf::Text& t, unsigned size, float y, sf::Color col) {
        t.setFont(font_);
        t.setCharacterSize(size);
        t.setFillColor(col);
        t.setPosition(20.0f, y);
    };
    setupText(hp_text_,       36, 60.0f,  sf::Color(220, 220, 220));
    setupText(money_text_,    36, 110.0f, sf::Color(220, 220, 100));
    setupText(wave_text_,     36, 160.0f, sf::Color(220, 220, 220));
    setupText(status_text_,   24, 220.0f, sf::Color(180, 220, 180));
    setupText(last_err_text_, 20, 1010.0f, sf::Color(230, 100, 100));
}

// helpers update/render

sf::Vector2f GameScene::cellToPx(int row, int col) const {
    return { GRID_X + col * CELL_PX, row * CELL_PX };
}

sf::Vector2f GameScene::cellCenter(float colF, float rowF) const {
    return { GRID_X + (colF + 0.5f) * CELL_PX, (rowF + 0.5f) * CELL_PX };
}

void GameScene::cycleGameSpeed() {
    if (game_speed_ == 0) {
        if (wave_running_ || game_.allWavesDone() || game_.isGameOver()) {
            return;
        }
        game_.startWave();
        wave_running_ = true;
        game_speed_   = 1;
    }
    else if (game_speed_ == 1) game_speed_ = 2;
    else if (game_speed_ == 2) game_speed_ = 4;
    else                       game_speed_ = 1;   // dupa 4x revine la 1
    updateFFLabel();
}

void GameScene::updateFFLabel() {
    if (!has_font_) return;
    if (game_speed_ == 0) {
        ff_button_.label.setString("START");
    } else {
        ff_button_.label.setString(std::to_string(game_speed_) + "x");
    }
}

void GameScene::updateShopButtonLabel() {
    if (!has_font_) return;
    std::string base = shop_.visible() ? "CLOSE SHOP" : "OPEN SHOP";
    if (shop_.tokenCount() > 0) {
        base += "  (" + std::to_string(shop_.tokenCount()) + " token";
        base += (shop_.tokenCount() == 1 ? ")" : "s)");
    }
    shop_button_.label.setString(base);
}

void GameScene::refreshShopForCurrentWave() {
    int wave_num = game_.getWaveNumber();
    const MapSpec& map = registry_.getMap("default");
    bool offers_major = true;
    if (wave_num >= 1 && wave_num <= static_cast<int>(map.wave_ids.size())) {
        offers_major = registry_.getWave(map.wave_ids[wave_num - 1]).offers_major;
    }
    shop_.refresh(game_.mutableRng(), offers_major);
    updateShopButtonLabel();
}

void GameScene::saveNow() {
    SaveData d;
    game_.serializeTo(d);
    shop_.serializeTo(d);
    try {
        SaveManager::write(d);
    } catch (const GameException& err) {
        last_error_ = std::string("Save fail: ") + err.what();
        err_clock_.restart();
    }
}

void GameScene::checkGameOver() {
    if (game_over_triggered_) return;

    bool defeat  = game_.isGameOver() || game_.getCurrentWave().bossEscaped();
    bool victory = game_.allWavesDone();
    if (defeat) victory = false;

    if (!defeat && !victory) return;

    game_over_triggered_ = true;
    int wave_reached = std::min(game_.getWaveNumber(), game_.getMaxWaves());
    manager_.requestReplaceAll(std::make_unique<GameOverScene>(
        manager_, registry_, victory, game_.getMapId(),
        wave_reached, game_.getTotalKills(), game_.getTotalMoneyEarned()));
}

bool GameScene::tryApplyActiveTokenOnTower(int col, int row) {
    const EvolutionToken* tok = shop_.activeToken();
    if (!tok) return false;

    // Cauta tower-ul la (col,row)
    Tower* target = nullptr;
    for (const auto& t : game_.getTowers()) {
        if (t->getX() == col && t->getY() == row) {
            target = t.get();
            break;
        }
    }
    if (!target) return false;
    if (!target->supports(tok->ability)) return false;

    try {
        // KNOCKBACK_EVERY_3 pastreaza dynamic_cast in AbilityEvolution::apply (T2 cerinta).
        // Restul virtual applyAbility(AbilityType).
        AbilityEvolution applier(tok->name, 0, tok->rarity, tok->ability);
        GlobalStatBuffs& buffs = game_.mutableBuffs();
        EvolutionContext ctx{ &buffs, target->getTypeKey(), target };
        applier.apply(ctx);
    } catch (const GameException& err) {
        last_error_ = err.what();
        err_clock_.restart();
        return false;
    }

    target->recordTokenInvestment(tok->cost);

    shop_.consumeActiveToken();
    token_apply_mode_ = false;
    updateShopButtonLabel();
    return true;
}

const Tower* GameScene::findTowerAt(int col, int row) const {
    for (const auto& t : game_.getTowers()) {
        if (t->getX() == col && t->getY() == row) return t.get();
    }
    return nullptr;
}

void GameScene::sellSelectedTower() {
    const Tower* t = findTowerAt(selected_tower_col_, selected_tower_row_);
    if (!t) return;
    int refund = game_.sellTower(selected_tower_col_, selected_tower_row_);
    selected_tower_col_ = -1;
    selected_tower_row_ = -1;
    last_error_ = "Sold for " + std::to_string(refund) + " cr";
    err_clock_.restart();
    saveNow();
}

void GameScene::renderTowerInfoPanel(sf::RenderWindow& window, const Tower* tower) {
    if (!tower || !has_font_) return;

    constexpr float PANEL_W = 300.0f;
    constexpr float PANEL_H = 360.0f;

    sf::Vector2f tower_center = cellCenter(static_cast<float>(tower->getX()),
                                           static_cast<float>(tower->getY()));
    float panel_x;
    if (tower->getX() < CELLS / 2) {
        panel_x = tower_center.x + CELL_PX * 1.2f;
    } else {
        panel_x = tower_center.x - CELL_PX * 1.2f - PANEL_W;
    }
    float panel_y = std::clamp(tower_center.y - PANEL_H / 2.0f,
                               10.0f, static_cast<float>(WIN_H) - PANEL_H - 10.0f);

    sf::RectangleShape bg(sf::Vector2f(PANEL_W, PANEL_H));
    bg.setPosition(panel_x, panel_y);
    bg.setFillColor(sf::Color(25, 28, 40, 235));
    bg.setOutlineColor(sf::Color(180, 200, 230));
    bg.setOutlineThickness(2.0f);
    window.draw(bg);

    const auto& spec = tower->spec();
    const auto& tb   = game_.getBuffs().for_type(tower->getTypeKey());

    auto drawLine = [&](const std::string& s, unsigned size, float y_offset, sf::Color col) {
        sf::Text t;
        t.setFont(font_);
        t.setString(s);
        t.setCharacterSize(size);
        t.setFillColor(col);
        t.setPosition(panel_x + 12.0f, panel_y + y_offset);
        window.draw(t);
    };

    drawLine(spec.display_name, 22, 10.0f, sf::Color(220, 230, 255));
    drawLine("(" + std::to_string(tower->getX()) + "," + std::to_string(tower->getY()) + ")",
             14, 40.0f, sf::Color(150, 170, 200));

    float y = 70.0f;
    auto stat = [&](const std::string& label, float base, float pct) {
        float eff = base * (1.0f + pct);
        std::string s = label + ": " + std::to_string(static_cast<int>(eff * 100) / 100.0f);
        if (pct > 0.0f) s += " (+" + std::to_string(static_cast<int>(pct * 100)) + "%)";
        drawLine(s, 16, y, sf::Color(200, 220, 200));
        y += 22.0f;
    };
    if (spec.damage       > 0.0f) stat("Damage",   spec.damage,       tb.damage_pct);
    if (spec.range        > 0.0f) stat("Range",    spec.range,        tb.range_pct);
    if (spec.attack_speed > 0.0f) stat("AtkSpeed", spec.attack_speed, tb.attack_speed_pct);
    if (spec.max_hp       > 0.0f) stat("MaxHP",    spec.max_hp,       tb.max_hp_pct);
    if (spec.regen_rate   > 0.0f) stat("Regen",    spec.regen_rate,   tb.regen_pct);
    if (spec.income_per_wave > 0) stat("Income",   static_cast<float>(spec.income_per_wave), tb.income_pct);

    y += 10.0f;
    drawLine("Evolutions:", 16, y, sf::Color(180, 200, 230));
    y += 22.0f;
    auto applied = tower->getAppliedAbilities();
    if (applied.empty()) {
        drawLine("(none)", 14, y, sf::Color(120, 140, 160));
        y += 20.0f;
    } else {
        for (auto ab : applied) {
            drawLine("- " + std::string(abilityToString(ab)), 14, y, sf::Color(220, 200, 140));
            y += 20.0f;
        }
    }

    int refund = static_cast<int>(0.75f * spec.cost) + static_cast<int>(0.5f * tower->getTokenInvestment());
    sf::RectangleShape sell(sf::Vector2f(PANEL_W - 24.0f, 40.0f));
    sell.setPosition(panel_x + 12.0f, panel_y + PANEL_H - 52.0f);
    sell.setFillColor(sf::Color(140, 80, 80));
    sell.setOutlineColor(sf::Color::White);
    sell.setOutlineThickness(2.0f);
    window.draw(sell);
    drawLine("SELL  " + std::to_string(refund) + " cr  [S]", 20,
             PANEL_H - 44.0f, sf::Color::White);
}

void GameScene::drawRangeCircle(sf::RenderWindow& window, sf::Vector2f center, float radiusPx,
                                sf::Color fill, sf::Color outline) const {
    sf::CircleShape c(radiusPx);
    c.setFillColor(fill);
    c.setOutlineColor(outline);
    c.setOutlineThickness(2.0f);
    c.setPosition(center.x - radiusPx, center.y - radiusPx);
    window.draw(c);
}
//ajutor cu AI
void GameScene::tickProjectileFiring(float dt) {
    while (last_fire_time_.size() < game_.getTowers().size())
        last_fire_time_.push_back(game_clock_.getElapsedTime().asSeconds());

    if (!wave_running_) return;

    float now = game_clock_.getElapsedTime().asSeconds();
    const auto& towers  = game_.getTowers();
    const auto& enemies = game_.getCurrentWave().getActiveEnemies();
    for (size_t i = 0; i < towers.size(); i++) {
        char ch        = towers[i]->getDisplayChar();
        float interval = fireIntervalForChar(ch);
        if (interval <= 0.0f) continue;
        if (now - last_fire_time_[i] < interval) continue;

        float range    = towers[i]->getRange();
        float bestDist = 1e9f;
        const Enemy* nearest = nullptr;
        for (const auto& e : enemies) {
            if (!e.isAlive()) continue;
            float dx = e.getX() - static_cast<float>(towers[i]->getX());
            float dy = e.getY() - static_cast<float>(towers[i]->getY());
            float d  = std::sqrt(dx*dx + dy*dy);
            if (d <= range && d < bestDist) { bestDist = d; nearest = &e; }
        }

        if (nearest) {
            float ps = projectileSpeedForChar(ch);
            auto [ix, iy] = calculateIntercept(towers[i]->getX(), towers[i]->getY(),
                                               nearest->getX(), nearest->getY(),
                                               nearest->getVelocityX(), nearest->getVelocityY(),
                                               ps);
            Projectile p;
            p.start    = cellCenter(static_cast<float>(towers[i]->getX()),
                                    static_cast<float>(towers[i]->getY()));
            p.end      = cellCenter(ix, iy);
            p.progress = 0.0f;
            float ddx  = ix - static_cast<float>(towers[i]->getX());
            float ddy  = iy - static_cast<float>(towers[i]->getY());
            float dist = std::sqrt(ddx * ddx + ddy * ddy);
            p.duration = (ps > 0.0f) ? (dist / ps) : 0.3f;
            if (p.duration < 0.05f) p.duration = 0.05f;
            p.color    = (ch == 'A') ? sf::Color(150, 220, 255) : sf::Color(255, 240, 100);

            // FIRE_TRAIL visual: doar tower-ele cu evolutia aplicata lasa dara.
            auto abilities = towers[i]->getAppliedAbilities();
            p.has_fire_trail = std::find(abilities.begin(), abilities.end(),
                                         AbilityType::FIRE_TRAIL) != abilities.end();

            projectiles_.push_back(p);
            last_fire_time_[i] = now;
        }
    }

    // Trail spawn: pentru fiecare proiectila cu FIRE_TRAIL, spawnam o particula
    // la pozitia curenta la fiecare TRAIL_SPAWN_INTERVAL secunde.
    constexpr float TRAIL_SPAWN_INTERVAL = 0.03f;
    constexpr float TRAIL_LIFETIME       = 3.0f;
    for (auto& p : projectiles_) {
        p.progress += dt / p.duration;
        if (!p.has_fire_trail) continue;
        p.trail_spawn_timer -= dt;
        if (p.trail_spawn_timer <= 0.0f) {
            sf::Vector2f pos = p.start + (p.end - p.start) * p.progress;
            trail_particles_.push_back({pos, 0.0f, TRAIL_LIFETIME});
            p.trail_spawn_timer = TRAIL_SPAWN_INTERVAL;
        }
    }
    projectiles_.erase(std::remove_if(projectiles_.begin(), projectiles_.end(),
        [](const Projectile& p) { return p.progress >= 1.0f; }), projectiles_.end());

    // Aging particule + cleanup pe cele expirate.
    for (auto& tp : trail_particles_) tp.age += dt;
    trail_particles_.erase(std::remove_if(trail_particles_.begin(), trail_particles_.end(),
        [](const TrailParticle& tp) { return tp.age >= tp.lifetime; }),
        trail_particles_.end());
}


// Scene interface


void GameScene::update(float dt) {
    if (dt > 0.1f) dt = 0.1f;
    float gdt = dt * static_cast<float>(game_speed_);

    if (wave_running_) {
        game_.tickWave(gdt);
        if (!game_.isWaveActive()) {
            // Refresh shop inainte de endWave (waveNumber inca pointeaza la wave-ul terminat,
            // si avem nevoie de WaveSpec.offers_major al acelui wave).
            refreshShopForCurrentWave();
            game_.endWave();
            wave_running_ = false;
            game_speed_ = 0;
            updateFFLabel();
            // Save trigger (anti-RNG manipulation): salvam dupa wave end + dupa orice oferta shop noua.
            saveNow();
        }

        // Boss kill detection
        if (game_.getWaveNumber() == game_.getMaxWaves() || boss_was_active_) {
            bool bossNow = false;
            for (const auto& e : game_.getCurrentWave().getActiveEnemies()) {
                if (e.isAlive() && e.getName() == "ILOVEYOU") { bossNow = true; break; }
            }
            if (bossNow) boss_was_active_ = true;
            // Boss kill = activare endless mode (in loc de victory imediat).
            if (boss_was_active_ && !bossNow
                && !game_.isEndlessActive()
                && !game_.isGameOver()
                && !game_.getCurrentWave().bossEscaped()) {
                game_.enableEndless();
                wave_running_ = false;
                game_speed_   = 0;
                updateFFLabel();
                last_error_ = "BOSS DOWN - ENDLESS MODE active";
                err_clock_.restart();
            }
        }
    }

    tickProjectileFiring(gdt);

    // Tranzitie deferred la GameOverScene (defeat / victory).
    checkGameOver();
}

void GameScene::render(sf::RenderWindow& window) {
    window.clear(sf::Color(20, 20, 30));
    window.draw(ui_left_);
    window.draw(ui_right_);
    window.draw(grid_quads_);
    window.draw(grid_lines_);

    sf::Vector2i mouseI = sf::Mouse::getPosition(window);
    float mx = static_cast<float>(mouseI.x);
    float my = static_cast<float>(mouseI.y);
    int hoverCol = static_cast<int>((mx - static_cast<float>(GRID_X)) / CELL_PX);
    int hoverRow = static_cast<int>(my / CELL_PX);
    bool inGrid = (hoverCol >= 0 && hoverCol < CELLS && hoverRow >= 0 && hoverRow < CELLS);

    const Tower* hoveredTower = nullptr;
    if (inGrid) {
        for (const auto& t : game_.getTowers()) {
            if (t->getX() == hoverCol && t->getY() == hoverRow) {
                hoveredTower = t.get();
                break;
            }
        }
    }

    if (hoveredTower && hoveredTower->getRange() > 0.0f) {
        sf::Vector2f c = cellCenter(static_cast<float>(hoveredTower->getX()),
                                    static_cast<float>(hoveredTower->getY()));
        drawRangeCircle(window, c, hoveredTower->effectiveRange(game_.getBuffs()) * CELL_PX,
                        sf::Color(255, 255, 255, 35),
                        sf::Color(255, 255, 255, 130));
    }

    if (!hoveredTower && inGrid && selected_type_ != 0) {
        static const char* keys[] = {"antivirus","adblocker","honeypot","firewall","bytecoinminer"};
        const std::string& key = keys[selected_type_ - 1];
        float baseR = registry_.getTower(key).range;
        if (baseR > 0.0f) {
            float effR = baseR * (1.0f + game_.getBuffs().for_type(key).range_pct);
            sf::Vector2f c = cellCenter(static_cast<float>(hoverCol),
                                        static_cast<float>(hoverRow));
            drawRangeCircle(window, c, effR * CELL_PX,
                            sf::Color(255, 255, 255, 25),
                            sf::Color(255, 255, 255, 100));
        }
        sf::RectangleShape hl(sf::Vector2f(CELL_PX, CELL_PX));
        hl.setPosition(cellToPx(hoverRow, hoverCol));
        hl.setFillColor(sf::Color(255, 255, 255, 60));
        window.draw(hl);
    }

    // Daca avem token activ, marcheaza vizual toate tower-ele cu halo verde (compatibile)
    // sau halo rosu slab (incompatibile). Desenat inainte de tower sprites ca sa fie in spate.
    if (token_apply_mode_ && shop_.hasActiveToken()) {
        const EvolutionToken* tok = shop_.activeToken();
        for (const auto& t : game_.getTowers()) {
            sf::Vector2f c = cellCenter(static_cast<float>(t->getX()),
                                        static_cast<float>(t->getY()));
            if (t->supports(tok->ability)) {
                drawRangeCircle(window, c, CELL_PX * 0.55f,
                                sf::Color( 80, 255,  80,  70),
                                sf::Color( 80, 255,  80, 220));
            } else {
                drawRangeCircle(window, c, CELL_PX * 0.45f,
                                sf::Color(150,  60,  60,  40),
                                sf::Color(150,  60,  60, 120));
            }
        }
    }

    for (const auto& tower : game_.getTowers()) {
        int idx = Sprites::towerIndex(tower->getDisplayChar());
        sf::Vector2f c = cellCenter(static_cast<float>(tower->getX()),
                                    static_cast<float>(tower->getY()));
        if (idx >= 0 && sprites_.towerOk[idx]) {
            sf::Sprite spr(sprites_.towerTex[idx]);
            sf::Vector2u sz = sprites_.towerTex[idx].getSize();
            spr.setOrigin(sz.x / 2.0f, sz.y / 2.0f);
            spr.setScale(CELL_PX / sz.x, CELL_PX / sz.y);
            spr.setPosition(c);
            window.draw(spr);
        } else {
            sf::CircleShape t(CELL_PX * 0.35f);
            t.setFillColor(towerColor(tower->getDisplayChar()));
            t.setOutlineColor(sf::Color::Black);
            t.setOutlineThickness(2.0f);
            t.setPosition(c.x - CELL_PX * 0.35f, c.y - CELL_PX * 0.35f);
            window.draw(t);
        }
    }


    // Alpha = (1 - age/lifetime) ca sa avem fade-out neted.
    if (sprites_.fireTrailOk) {
        sf::Vector2u sz = sprites_.fireTrailTex.getSize();
        constexpr float TRAIL_PX = 16.0f;
        for (const auto& tp : trail_particles_) {
            float t = tp.age / tp.lifetime;
            if (t > 1.0f) t = 1.0f;
            sf::Uint8 alpha = static_cast<sf::Uint8>(255.0f * (1.0f - t));
            sf::Sprite spr(sprites_.fireTrailTex);
            spr.setOrigin(sz.x / 2.0f, sz.y / 2.0f);
            spr.setScale(TRAIL_PX / sz.x, TRAIL_PX / sz.y);
            spr.setPosition(tp.pos);
            spr.setColor(sf::Color(255, 255, 255, alpha));
            window.draw(spr);
        }
    }

    for (const auto& enemy : game_.getCurrentWave().getActiveEnemies()) {
        if (!enemy.isAlive()) continue;
        sf::Vector2f c = cellCenter(enemy.getX(), enemy.getY());
        float ds = Sprites::displayScale(enemy.getName());
        const sf::Texture* tex = sprites_.textureFor(enemy);
        if (tex) {
            sf::Sprite spr(*tex);
            sf::Vector2u sz = tex->getSize();
            spr.setOrigin(sz.x / 2.0f, sz.y / 2.0f);
            spr.setScale(ds * CELL_PX / sz.x, ds * CELL_PX / sz.y);
            spr.setPosition(c);
            window.draw(spr);
        } else {
            float radius = CELL_PX * 0.25f * ds;
            sf::CircleShape e(radius);
            e.setFillColor(sf::Color(220, 50, 50));
            e.setOutlineColor(sf::Color::Black);
            e.setOutlineThickness(1.5f);
            e.setPosition(c.x - radius, c.y - radius);
            window.draw(e);
        }
    }

    for (const auto& p : projectiles_) {
        sf::Vector2f pos = p.start + (p.end - p.start) * p.progress;
        sf::CircleShape dot(6.0f);
        dot.setFillColor(p.color);
        dot.setOutlineColor(sf::Color::Black);
        dot.setOutlineThickness(1.0f);
        dot.setPosition(pos.x - 6.0f, pos.y - 6.0f);
        window.draw(dot);
    }

    for (const auto& btn : tower_buttons_) {
        window.draw(btn.rect);
        window.draw(btn.label);
        if (btn.actionId == selected_type_) {
            sf::RectangleShape sel = btn.rect;
            sel.setFillColor(sf::Color::Transparent);
            sel.setOutlineColor(sf::Color::White);
            sel.setOutlineThickness(4.0f);
            window.draw(sel);
        }
    }

    // FF button
    window.draw(ff_button_.rect);
    if (has_font_) window.draw(ff_button_.label);

    // Shop button
    window.draw(shop_button_.rect);
    if (has_font_) window.draw(shop_button_.label);

    // Token icons sub Shop button (max 3 sloturi, doar daca exista tokens).
    const auto& tokens = shop_.getTokens();
    int hovered_token = -1;
    for (size_t i = 0; i < std::min<size_t>(tokens.size(), 3); ++i) {
        auto r = tokenSlotRect(static_cast<int>(i));
        sf::RectangleShape slot({r.width, r.height});
        slot.setPosition(r.left, r.top);
        slot.setFillColor(rarityToColor(tokens[i].rarity));
        bool is_active = (shop_.activeTokenIndex() == static_cast<int>(i));
        slot.setOutlineColor(is_active ? sf::Color(80, 255, 80) : sf::Color::Black);
        slot.setOutlineThickness(is_active ? 4.0f : 2.0f);
        window.draw(slot);

        if (mx >= r.left && mx <= r.left + r.width &&
            my >= r.top  && my <= r.top + r.height) {
            hovered_token = static_cast<int>(i);
        }
    }

    if (has_font_) {
        hp_text_.setString("HP:     " + std::to_string(game_.getPlayerHP()));
        money_text_.setString("$:      " + std::to_string(game_.getMoney()));
        wave_text_.setString("Wave:   "
            + std::to_string(std::min(game_.getWaveNumber(), game_.getMaxWaves()))
            + "/" + std::to_string(game_.getMaxWaves()));

        std::string status;
        if (game_.isGameOver())            status = "GAME OVER (ESC -> menu)";
        else if (game_.isEndlessActive())  status = "ENDLESS MODE - wave " + std::to_string(game_.getWaveNumber());
        else if (game_.allWavesDone())     status = "VICTORY! (ESC -> menu)";
        else if (token_apply_mode_)        status = "Apply token: click pe tower verde\nclick in afara = cancel";
        else if (wave_running_)            status = "Wave in curs...";
        else if (selected_type_ != 0)      status = "Click pe grid sa plasezi";
        else                                status = "SPACE = start wave\nU = undo\nESC = menu\nF = fast-forward";
        status_text_.setString(status);

        if (!last_error_.empty() && err_clock_.getElapsedTime().asSeconds() < 3.0f) {
            last_err_text_.setString(last_error_);
            window.draw(last_err_text_);
        }

        window.draw(hp_text_);
        window.draw(money_text_);
        window.draw(wave_text_);
        window.draw(status_text_);
    }

    // Tower info panel (apare cand player a click pe un tower).
    renderTowerInfoPanel(window, findTowerAt(selected_tower_col_, selected_tower_row_));

    // Token hover popup (langa iconita, in dreapta). Desenat inainte de shop overlay
    // ca sa fie sub el cand shop e deschis.
    if (hovered_token >= 0 && has_font_ && hovered_token < static_cast<int>(tokens.size())) {
        const auto& tk = tokens[hovered_token];
        auto r = tokenSlotRect(hovered_token);
        sf::RectangleShape pop(sf::Vector2f(260.0f, 80.0f));
        pop.setPosition(r.left + r.width + 8.0f, r.top);
        pop.setFillColor(sf::Color(25, 28, 40, 230));
        pop.setOutlineColor(rarityToColor(tk.rarity));
        pop.setOutlineThickness(2.0f);
        window.draw(pop);

        sf::Text t;
        t.setFont(font_);
        t.setFillColor(sf::Color(220, 230, 255));
        t.setCharacterSize(18);
        t.setString(tk.name);
        t.setPosition(r.left + r.width + 16.0f, r.top + 6.0f);
        window.draw(t);
        t.setCharacterSize(14);
        t.setString(std::string("Ability: ") + abilityToString(tk.ability));
        t.setPosition(r.left + r.width + 16.0f, r.top + 30.0f);
        window.draw(t);
        t.setString("Click to apply on a tower");
        t.setFillColor(sf::Color(160, 200, 160));
        t.setPosition(r.left + r.width + 16.0f, r.top + 52.0f);
        window.draw(t);
    }

    // Shop overlay desenat ULTIMUL (peste tot restul), doar daca visible.
    shop_.render(window, game_.getMoney());
}

void GameScene::handleEvent(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape) {
            // ESC = pauza (push overlay). Game-ul inghetza (SceneManager update doar top).
            manager_.requestPush(std::make_unique<PauseScene>(manager_, *this, registry_));
            return;
        }
        if (event.key.code == sf::Keyboard::Space) {
            // SPACE = identic cu F: pornire wave la 0->1, fastforward 1->2->4->1.
            cycleGameSpeed();
        }
        if (event.key.code == sf::Keyboard::U) {
            if (game_.restoreSnapshot()) {
                last_error_ = ">>> Undo: revenit la wavestart " + std::to_string(game_.getWaveNumber());
                err_clock_.restart();
                wave_running_    = false;
                boss_was_active_ = false;
                game_speed_      = 0;
                updateFFLabel();
                projectiles_.clear();
                trail_particles_.clear();
                last_fire_time_.assign(game_.getTowers().size(),
                                       game_clock_.getElapsedTime().asSeconds());
            }
        }
        if (event.key.code == sf::Keyboard::F) {
            cycleGameSpeed();
        }
        if (event.key.code == sf::Keyboard::S && selected_tower_col_ >= 0) {
            sellSelectedTower();
        }
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Right) {
        selected_type_      = 0;
        selected_tower_col_ = -1;
        selected_tower_row_ = -1;
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left) {
        float mx = static_cast<float>(event.mouseButton.x);
        float my = static_cast<float>(event.mouseButton.y);

        // 1. Shop overlay are prioritate (modal pentru click-uri in interiorul lui).
        if (shop_.visible()) {
            std::string err;
            ShopPanel::ClickResult res = shop_.handleClick(mx, my,
                game_.mutableMoney(), game_.mutableWeight(), err);
            switch (res) {
                case ShopPanel::ClickResult::CLOSE:
                    shop_.setVisible(false);
                    token_apply_mode_ = false;
                    updateShopButtonLabel();
                    return;
                case ShopPanel::ClickResult::BUY_FAILED:
                    last_error_ = err;
                    err_clock_.restart();
                    return;
                case ShopPanel::ClickResult::BUY_OK_INSTANT:
                    // Mini sau Major STAT: aplicat instant pe buffs in handleClick.
                    saveNow();
                    return;
                case ShopPanel::ClickResult::BUY_OK_TOKEN:
                    updateShopButtonLabel();
                    saveNow();
                    return;
                case ShopPanel::ClickResult::TOKEN_PICKED:
                    token_apply_mode_ = true;
                    last_error_ = "Apply token: click pe tower compatibil (halo verde) sau in afara grid pentru cancel";
                    err_clock_.restart();
                    return;
                case ShopPanel::ClickResult::NONE:
                    // Click in afara overlay-ului (sau pe slot deja vandut) - cade jos.
                    break;
            }
        }

        // 1b. Token icon click → activate apply mode (in afara shopului).
        {
            const auto& tokens = shop_.getTokens();
            for (size_t i = 0; i < std::min<size_t>(tokens.size(), 3); ++i) {
                auto r = tokenSlotRect(static_cast<int>(i));
                if (mx >= r.left && mx <= r.left + r.width &&
                    my >= r.top  && my <= r.top + r.height) {
                    shop_.setActiveTokenIndex(static_cast<int>(i));
                    token_apply_mode_ = true;
                    last_error_ = "Apply token: click pe tower compatibil (halo verde) sau in afara grid pentru cancel";
                    err_clock_.restart();
                    return;
                }
            }
        }

        // 2. FF button.
        if (ff_button_.contains(mx, my)) {
            cycleGameSpeed();
            return;
        }

        // 3. Shop toggle button.
        if (shop_button_.contains(mx, my)) {
            shop_.toggle();
            if (!shop_.visible() && token_apply_mode_) {
                shop_.cancelActiveToken();
                token_apply_mode_ = false;
            }
            updateShopButtonLabel();
            return;
        }

        // 4. Tower buttons.
        bool clickedButton = false;
        for (const auto& btn : tower_buttons_) {
            if (btn.contains(mx, my)) {
                selected_type_ = btn.actionId;
                clickedButton  = true;
                break;
            }
        }

        if (!clickedButton) {
            int col = static_cast<int>((mx - GRID_X) / CELL_PX);
            int row = static_cast<int>(my / CELL_PX);
            bool inGrid = (col >= 0 && col < CELLS && row >= 0 && row < CELLS);

            if (token_apply_mode_) {
                bool applied = false;
                if (inGrid) {
                    applied = tryApplyActiveTokenOnTower(col, row);
                }
                if (!applied) {
                    shop_.cancelActiveToken();
                    token_apply_mode_ = false;
                    last_error_ = "Cancelled apply mode";
                    err_clock_.restart();
                }
                return;
            }

            if (inGrid) {
                const Tower* clicked_tower = findTowerAt(col, row);
                if (clicked_tower) {
                    selected_tower_col_ = col;
                    selected_tower_row_ = row;
                    return;
                }
                if (selected_type_ != 0) {
                    try {
                        game_.placeTower(selected_type_, col, row);
                        saveNow();
                    } catch (const GameException& err) {
                        last_error_ = err.what();
                        err_clock_.restart();
                    }
                    return;
                }
            }

            selected_tower_col_ = -1;
            selected_tower_row_ = -1;
        }
    }
}
