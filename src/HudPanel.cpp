#include "HudPanel.h"
#include "Game.h"
#include "ShopPanel.h"
#include "Tower.h"
#include "DataRegistry.h"
#include "GlobalStatBuffs.h"
#include "AbilityType.h"
#include "Palette.h"
#include "UiLayout.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

namespace {
    constexpr float INFO_PANEL_W = 300.0f;

    // Evolutiile aplicate, grupate cu contor ("DoubleShot x2") — asa se vad
    // TOATE intr-un panel compact, oricat de multe token-uri ai bagat.
    std::vector<std::pair<AbilityType, int>> groupedAbilities(const Tower& t) {
        std::vector<std::pair<AbilityType, int>> out;
        for (AbilityType ab : t.getAppliedAbilities()) {
            bool found = false;
            for (auto& [a, n] : out) {
                if (a == ab) { ++n; found = true; break; }
            }
            if (!found) out.push_back({ab, 1});
        }
        return out;
    }

    // Word-wrap simplu pentru tooltips (SFML nu are wrap nativ).
    std::vector<std::string> wrapText(const std::string& s, size_t max_chars) {
        std::vector<std::string> lines;
        std::istringstream iss(s);
        std::string cur, word;
        while (iss >> word) {
            if (!cur.empty() && cur.size() + 1 + word.size() > max_chars) {
                lines.push_back(cur);
                cur.clear();
            }
            if (!cur.empty()) cur += ' ';
            cur += word;
        }
        if (!cur.empty()) lines.push_back(cur);
        return lines;
    }

    // Geometria popup-ului de loterie (centrat).
    constexpr float LOTTO_W = 640.0f;
    constexpr float LOTTO_H = 320.0f;
    constexpr float LOTTO_X = (ui::WIN_W - LOTTO_W) / 2.0f;
    constexpr float LOTTO_Y = 330.0f;
    const sf::FloatRect LOTTO_REDEEM(LOTTO_X + (LOTTO_W - 260.0f) / 2.0f,
                                     LOTTO_Y + LOTTO_H - 70.0f, 260.0f, 50.0f);
    const sf::FloatRect LOTTO_CLOSE(LOTTO_X + LOTTO_W - 44.0f, LOTTO_Y + 8.0f,
                                    36.0f, 36.0f);
}

HudPanel::HudPanel(const sf::Font* font, const DataRegistry& registry)
    : font_(font) {
    ui_left_.setSize(sf::Vector2f(ui::GRID_X, ui::WIN_H));
    ui_left_.setFillColor(sf::Color(40, 40, 55));

    ui_right_.setSize(sf::Vector2f(ui::GRID_X, ui::WIN_H));
    ui_right_.setPosition(ui::GRID_X + ui::GRID_PX, 0);
    ui_right_.setFillColor(sf::Color(40, 40, 55));

    // Butoanele de tower: label-urile vin din TowerSpec (nume + cost), nu mai sunt
    // hardcodate. Daca schimbam costul in towers.json, butonul afiseaza corect.
    static const char* keys[] = {"antivirus", "adblocker", "honeypot", "firewall", "bytecoinminer"};
    float y = 60.0f;
    for (int type = 1; type <= 5; ++type) {
        const TowerSpec& spec = registry.getTower(keys[type - 1]);
        Button b;
        b.actionId = type;
        b.rect.setSize(sf::Vector2f(380.0f, 90.0f));
        b.rect.setPosition(ui::GRID_X + ui::GRID_PX + 20.0f, y);
        b.rect.setFillColor(palette::towerColor("ADHFM"[type - 1]));
        b.rect.setOutlineColor(sf::Color::Black);
        b.rect.setOutlineThickness(2.0f);
        if (font_) {
            b.label.setFont(*font_);
            b.label.setString(" " + spec.display_name + "  $" + std::to_string(spec.cost));
            b.label.setCharacterSize(22);
            b.label.setFillColor(sf::Color::Black);
            b.label.setPosition(ui::GRID_X + ui::GRID_PX + 40.0f, y + 28.0f);
        }
        tower_buttons_.push_back(std::move(b));
        y += 110.0f;
    }

    // Buton START / fast-forward (label setat per frame in renderFront).
    ff_button_.actionId = 99;
    ff_button_.rect.setSize(sf::Vector2f(380.0f, 80.0f));
    ff_button_.rect.setPosition(ui::GRID_X + ui::GRID_PX + 20.0f, 900.0f);
    ff_button_.rect.setFillColor(sf::Color(60, 110, 80));
    ff_button_.rect.setOutlineColor(sf::Color::Black);
    ff_button_.rect.setOutlineThickness(2.0f);
    if (font_) {
        ff_button_.label.setFont(*font_);
        ff_button_.label.setCharacterSize(36);
        ff_button_.label.setFillColor(sf::Color::White);
        ff_button_.label.setPosition(ui::GRID_X + ui::GRID_PX + 40.0f, 920.0f);
    }

    // Buton shop (label setat per frame: OPEN/CLOSE + numar de tokens).
    shop_button_.actionId = 100;
    shop_button_.rect.setSize(sf::Vector2f(380.0f, 80.0f));
    shop_button_.rect.setPosition(20.0f, 420.0f);
    shop_button_.rect.setFillColor(sf::Color(80, 60, 130));
    shop_button_.rect.setOutlineColor(sf::Color::Black);
    shop_button_.rect.setOutlineThickness(2.0f);
    if (font_) {
        shop_button_.label.setFont(*font_);
        shop_button_.label.setCharacterSize(26);
        shop_button_.label.setFillColor(sf::Color::White);
        shop_button_.label.setPosition(50.0f, 440.0f);
    }

    if (font_) {
        auto setupText = [&](sf::Text& t, unsigned size, float ty, sf::Color col) {
            t.setFont(*font_);
            t.setCharacterSize(size);
            t.setFillColor(col);
            t.setPosition(20.0f, ty);
        };
        setupText(hp_text_,     36, 60.0f,   sf::Color(220, 220, 220));
        setupText(money_text_,  36, 110.0f,  sf::Color(220, 220, 100));
        setupText(wave_text_,   36, 160.0f,  sf::Color(220, 220, 220));
        setupText(status_text_, 24, 220.0f,  sf::Color(180, 220, 180));
        setupText(err_text_,    20, 1010.0f, sf::Color(230, 100, 100));
    }
}

sf::FloatRect HudPanel::tokenSlotRect(int i) {
    return { 20.0f + i * 65.0f, 520.0f, 50.0f, 50.0f };
}

sf::Vector2f HudPanel::infoPanelPos(const Tower& tower) {
    sf::Vector2f tower_center = ui::cellCenter(static_cast<float>(tower.getX()),
                                               static_cast<float>(tower.getY()));
    float panel_h = infoPanelHeight(tower);
    float panel_x;
    if (tower.getX() < ui::CELLS / 2) {
        panel_x = tower_center.x + ui::CELL_PX * 1.2f;
    } else {
        panel_x = tower_center.x - ui::CELL_PX * 1.2f - INFO_PANEL_W;
    }
    float panel_y = std::clamp(tower_center.y - panel_h / 2.0f,
                               10.0f, static_cast<float>(ui::WIN_H) - panel_h - 10.0f);
    return { panel_x, panel_y };
}

float HudPanel::infoPanelHeight(const Tower& tower) {
    const auto& spec = tower.spec();
    int stats = 0;
    if (spec.damage          > 0.0f) ++stats;
    if (spec.range           > 0.0f) ++stats;
    if (spec.attack_speed    > 0.0f) ++stats;
    if (spec.max_hp          > 0.0f) ++stats;
    if (spec.regen_rate      > 0.0f) ++stats;
    if (spec.income_per_wave > 0)    ++stats;

    int evo_lines = static_cast<int>(groupedAbilities(tower).size());
    if (tower.mythicBadge()) ++evo_lines;
    if (evo_lines == 0) evo_lines = 1;   // linia "(none)"

    float h = 70.0f + stats * 22.0f + 10.0f + 22.0f + evo_lines * 20.0f + 14.0f;
    if (tower.isMovable())        h += 38.0f;   // butonul MOVE
    if (spec.attack_speed > 0.0f) h += 38.0f;   // butonul TARGET
    h += 52.0f + 10.0f;                          // butonul SELL + padding
    return std::max(h, 240.0f);
}

// Butoanele sunt ancorate de marginea de jos a panoului (inaltime dinamica).
float HudPanel::sellButtonY(const Tower& tower)   { return infoPanelHeight(tower) - 52.0f; }
float HudPanel::targetButtonY(const Tower& tower) { return infoPanelHeight(tower) - 92.0f; }
float HudPanel::moveButtonY(const Tower& tower) {
    // Daca turnul nu trage, slotul de TARGET nu exista si MOVE coboara in locul lui.
    return infoPanelHeight(tower) - (tower.spec().attack_speed > 0.0f ? 130.0f : 92.0f);
}

HudPanel::Hit HudPanel::hitTest(float mx, float my, int tokenCount) const {
    // Token slots inaintea butoanelor (acelasi ordin ca handleEvent-ul original).
    for (int i = 0; i < std::min(tokenCount, 3); ++i) {
        auto r = tokenSlotRect(i);
        if (mx >= r.left && mx <= r.left + r.width &&
            my >= r.top  && my <= r.top + r.height) {
            return { Hit::Kind::TOKEN_SLOT, i };
        }
    }
    if (ff_button_.contains(mx, my))   return { Hit::Kind::FF, 0 };
    if (shop_button_.contains(mx, my)) return { Hit::Kind::SHOP_TOGGLE, 0 };
    for (const auto& btn : tower_buttons_) {
        if (btn.contains(mx, my)) return { Hit::Kind::TOWER_BUTTON, btn.actionId };
    }
    return {};
}

bool HudPanel::sellButtonContains(float mx, float my, const Tower& tower) const {
    sf::Vector2f p = infoPanelPos(tower);
    sf::FloatRect sell(p.x + 12.0f, p.y + sellButtonY(tower),
                       INFO_PANEL_W - 24.0f, 40.0f);
    return sell.contains(mx, my);
}

bool HudPanel::targetButtonContains(float mx, float my, const Tower& tower) const {
    if (tower.spec().attack_speed <= 0.0f) return false;
    sf::Vector2f p = infoPanelPos(tower);
    sf::FloatRect btn(p.x + 12.0f, p.y + targetButtonY(tower),
                      INFO_PANEL_W - 24.0f, 32.0f);
    return btn.contains(mx, my);
}

bool HudPanel::moveButtonContains(float mx, float my, const Tower& tower) const {
    if (!tower.isMovable()) return false;
    sf::Vector2f p = infoPanelPos(tower);
    sf::FloatRect btn(p.x + 12.0f, p.y + moveButtonY(tower),
                      INFO_PANEL_W - 24.0f, 32.0f);
    return btn.contains(mx, my);
}

void HudPanel::renderBackground(sf::RenderWindow& window) const {
    window.draw(ui_left_);
    window.draw(ui_right_);
}

void HudPanel::renderFront(sf::RenderWindow& window, const Game& game,
                           const ShopPanel& shop, const Frame& frame) {
    for (const auto& btn : tower_buttons_) {
        window.draw(btn.rect);
        if (font_) window.draw(btn.label);
        if (btn.actionId == frame.selected_type) {
            sf::RectangleShape sel = btn.rect;
            sel.setFillColor(sf::Color::Transparent);
            sel.setOutlineColor(sf::Color::White);
            sel.setOutlineThickness(4.0f);
            window.draw(sel);
        }
    }

    if (font_) {
        ff_button_.label.setString(frame.game_speed == 0
                                       ? "START"
                                       : std::to_string(frame.game_speed) + "x");
        std::string shopLabel = shop.visible() ? "CLOSE SHOP" : "OPEN SHOP";
        if (shop.tokenCount() > 0) {
            shopLabel += "  (" + std::to_string(shop.tokenCount()) + " token";
            shopLabel += (shop.tokenCount() == 1 ? ")" : "s)");
        }
        shop_button_.label.setString(shopLabel);
    }
    window.draw(ff_button_.rect);
    if (font_) window.draw(ff_button_.label);
    window.draw(shop_button_.rect);
    if (font_) window.draw(shop_button_.label);

    // Sloturi token (max 3): culoare dupa raritate, contur verde pe cel activ.
    const auto& tokens = shop.getTokens();
    for (size_t i = 0; i < std::min<size_t>(tokens.size(), 3); ++i) {
        auto r = tokenSlotRect(static_cast<int>(i));
        sf::RectangleShape slot({r.width, r.height});
        slot.setPosition(r.left, r.top);
        slot.setFillColor(palette::rarityColor(tokens[i].rarity));
        bool is_active = (shop.activeTokenIndex() == static_cast<int>(i));
        slot.setOutlineColor(is_active ? sf::Color(80, 255, 80) : sf::Color::Black);
        slot.setOutlineThickness(is_active ? 4.0f : 2.0f);
        window.draw(slot);
    }

    if (font_) {
        // Count-up: contorul alearga exponential spre valoarea reala (~0.4s),
        // cu puls de culoare la schimbare. Banii se SIMT cand intra/ies.
        float dt = money_clock_.restart().asSeconds();
        int   actual = game.getMoney();
        if (shown_money_ < 0.0f) {
            shown_money_ = static_cast<float>(actual);
            last_money_  = actual;
        }
        if (actual != last_money_) {
            money_up_    = actual > last_money_;
            money_pulse_ = 0.35f;
            last_money_  = actual;
        }
        float gap = static_cast<float>(actual) - shown_money_;
        shown_money_ += gap * std::min(1.0f, dt * 8.0f);
        if (std::abs(gap) < 1.0f) shown_money_ = static_cast<float>(actual);
        if (money_pulse_ > 0.0f) money_pulse_ -= dt;

        float p = std::max(0.0f, money_pulse_ / 0.35f);
        sf::Color money_col = money_up_
            ? sf::Color(static_cast<sf::Uint8>(220 - 100 * p), 220,
                        static_cast<sf::Uint8>(100 * (1.0f - p)))
            : sf::Color(220 + static_cast<sf::Uint8>(35 * p),
                        static_cast<sf::Uint8>(220 - 120 * p),
                        static_cast<sf::Uint8>(100 * (1.0f - p)));
        money_text_.setFillColor(money_col);
        money_text_.setScale(1.0f + 0.12f * p, 1.0f + 0.12f * p);

        hp_text_.setString("HP:     " + std::to_string(game.getPlayerHP()));
        money_text_.setString("$:      "
            + std::to_string(static_cast<int>(std::lround(shown_money_))));
        wave_text_.setString("Wave:   "
            + std::to_string(std::min(game.getWaveNumber(), game.getMaxWaves()))
            + "/" + std::to_string(game.getMaxWaves()));
        status_text_.setString(frame.status);

        window.draw(hp_text_);
        window.draw(money_text_);
        window.draw(wave_text_);
        window.draw(status_text_);

        if (!frame.error.empty()) {
            err_text_.setString(frame.error);
            window.draw(err_text_);
        }
    }
}

void HudPanel::renderTowerInfo(sf::RenderWindow& window, const Tower* tower,
                               const GlobalStatBuffs& buffs, float mx, float my) const {
    if (!tower || !font_) return;

    sf::Vector2f p = infoPanelPos(*tower);
    float panel_x = p.x;
    float panel_y = p.y;
    float panel_h = infoPanelHeight(*tower);

    sf::RectangleShape bg(sf::Vector2f(INFO_PANEL_W, panel_h));
    bg.setPosition(panel_x, panel_y);
    bg.setFillColor(sf::Color(25, 28, 40, 235));
    bg.setOutlineColor(sf::Color(180, 200, 230));
    bg.setOutlineThickness(2.0f);
    window.draw(bg);

    const auto& spec = tower->spec();
    const std::string& tk = tower->getTypeKey();

    auto drawLine = [&](const std::string& s, unsigned size, float y_offset, sf::Color col) {
        sf::Text t;
        t.setFont(*font_);
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
    if (spec.damage       > 0.0f) stat("Damage",   spec.damage,       buffs.pct(tk, "damage_pct"));
    if (spec.range        > 0.0f) stat("Range",    spec.range,        buffs.pct(tk, "range_pct"));
    if (spec.attack_speed > 0.0f) stat("AtkSpeed", spec.attack_speed, buffs.pct(tk, "attack_speed_pct"));
    if (spec.max_hp       > 0.0f) stat("MaxHP",    spec.max_hp,       buffs.pct(tk, "max_hp_pct"));
    if (spec.regen_rate   > 0.0f) stat("Regen",    spec.regen_rate,   buffs.pct(tk, "regen_pct"));
    if (spec.income_per_wave > 0) stat("Income",   static_cast<float>(spec.income_per_wave), buffs.pct(tk, "income_pct"));

    y += 10.0f;
    drawLine("Evolutions:", 16, y, sf::Color(180, 200, 230));
    y += 22.0f;

    // Hover pe o linie de evolutie => tooltip cu ce face efectiv (numere reale).
    const char* tooltip = nullptr;

    if (tower->mythicBadge()) {
        drawLine("* " + std::string(tower->mythicBadge()) + " *", 15, y,
                 palette::rarityColor(Evolution::Rarity::MYTHIC));
        y += 20.0f;
    }
    auto grouped = groupedAbilities(*tower);
    if (grouped.empty() && !tower->mythicBadge()) {
        drawLine("(none)", 14, y, sf::Color(120, 140, 160));
        y += 20.0f;
    }
    for (const auto& [ab, n] : grouped) {
        std::string line = "- " + std::string(abilityDisplayName(ab));
        if (n > 1) line += " x" + std::to_string(n);
        sf::FloatRect line_rect(panel_x + 8.0f, panel_y + y - 2.0f,
                                INFO_PANEL_W - 16.0f, 20.0f);
        bool hovered = line_rect.contains(mx, my);
        if (hovered) tooltip = abilityDescription(ab);
        drawLine(line, 14, y, hovered ? sf::Color(255, 245, 190)
                                      : sf::Color(220, 200, 140));
        y += 20.0f;
    }

    // Buton MOVE (doar la turnurile cu MOVABLE): intra in move mode.
    if (tower->isMovable()) {
        float by = moveButtonY(*tower);
        sf::RectangleShape mv(sf::Vector2f(INFO_PANEL_W - 24.0f, 32.0f));
        mv.setPosition(panel_x + 12.0f, panel_y + by);
        mv.setFillColor(sf::Color(60, 110, 80));
        mv.setOutlineColor(sf::Color(150, 220, 180));
        mv.setOutlineThickness(2.0f);
        window.draw(mv);
        drawLine("MOVE  [M]", 17, by + 6.0f, sf::Color::White);
    }

    // Buton TARGET (doar la turnurile care trag): cicleaza modurile de tintire.
    if (spec.attack_speed > 0.0f) {
        float by = targetButtonY(*tower);
        sf::RectangleShape tgt(sf::Vector2f(INFO_PANEL_W - 24.0f, 32.0f));
        tgt.setPosition(panel_x + 12.0f, panel_y + by);
        tgt.setFillColor(sf::Color(60, 80, 120));
        tgt.setOutlineColor(sf::Color(150, 180, 220));
        tgt.setOutlineThickness(2.0f);
        window.draw(tgt);
        drawLine(std::string("TARGET: ") + targetingModeName(tower->targeting()) + "  [T]",
                 17, by + 6.0f, sf::Color::White);
    }

    int refund = static_cast<int>(0.75f * spec.cost)
               + static_cast<int>(0.5f * tower->getTokenInvestment());
    float sy = sellButtonY(*tower);
    sf::RectangleShape sell(sf::Vector2f(INFO_PANEL_W - 24.0f, 40.0f));
    sell.setPosition(panel_x + 12.0f, panel_y + sy);
    sell.setFillColor(sf::Color(140, 80, 80));
    sell.setOutlineColor(sf::Color::White);
    sell.setOutlineThickness(2.0f);
    window.draw(sell);
    drawLine("SELL  " + std::to_string(refund) + " cr  [S]", 20,
             sy + 8.0f, sf::Color::White);

    // Tooltip-ul evolutiei, langa mouse, peste panel.
    if (tooltip) {
        auto lines = wrapText(tooltip, 42);
        float tw = 340.0f;
        float th = 14.0f + static_cast<float>(lines.size()) * 18.0f;
        float tx = std::min(mx + 18.0f, static_cast<float>(ui::WIN_W) - tw - 8.0f);
        float ty = std::min(my + 14.0f, static_cast<float>(ui::WIN_H) - th - 8.0f);
        sf::RectangleShape tip({tw, th});
        tip.setPosition(tx, ty);
        tip.setFillColor(sf::Color(15, 18, 28, 245));
        tip.setOutlineColor(sf::Color(220, 200, 140));
        tip.setOutlineThickness(1.5f);
        window.draw(tip);
        float ly = ty + 7.0f;
        for (const auto& l : lines) {
            sf::Text t;
            t.setFont(*font_);
            t.setString(l);
            t.setCharacterSize(13);
            t.setFillColor(sf::Color(230, 235, 245));
            t.setPosition(tx + 10.0f, ly);
            window.draw(t);
            ly += 18.0f;
        }
    }
}

void HudPanel::renderLotteryPopup(sf::RenderWindow& window) const {
    if (!font_) return;

    // Stil intentionat de reclama scam: galben tipator, contur rosu gros.
    sf::RectangleShape bg({LOTTO_W, LOTTO_H});
    bg.setPosition(LOTTO_X, LOTTO_Y);
    bg.setFillColor(sf::Color(250, 220, 80));
    bg.setOutlineColor(sf::Color(220, 40, 40));
    bg.setOutlineThickness(6.0f);
    window.draw(bg);

    auto text = [&](const std::string& s, unsigned size, float y, sf::Color col,
                    bool center = true) {
        sf::Text t;
        t.setFont(*font_);
        t.setString(s);
        t.setCharacterSize(size);
        t.setFillColor(col);
        t.setStyle(sf::Text::Bold);
        float x = LOTTO_X + 24.0f;
        if (center) {
            sf::FloatRect b = t.getLocalBounds();
            x = LOTTO_X + (LOTTO_W - b.width) / 2.0f;
        }
        t.setPosition(x, y);
        window.draw(t);
    };

    text("!!! CONGRATULATION !!!",            34, LOTTO_Y + 22.0f,  sf::Color(220, 40, 40));
    text("YOU HAVE WON THE BYTECOIN LOTTERY", 24, LOTTO_Y + 78.0f,  sf::Color(40, 40, 200));
    text("100% real no scam",                 16, LOTTO_Y + 158.0f, sf::Color(120, 100, 30));

    sf::RectangleShape redeem({LOTTO_REDEEM.width, LOTTO_REDEEM.height});
    redeem.setPosition(LOTTO_REDEEM.left, LOTTO_REDEEM.top);
    redeem.setFillColor(sf::Color(60, 180, 80));
    redeem.setOutlineColor(sf::Color::White);
    redeem.setOutlineThickness(3.0f);
    window.draw(redeem);
    text("REDEEM NOW", 26, LOTTO_REDEEM.top + 8.0f, sf::Color::White);

    sf::RectangleShape close({LOTTO_CLOSE.width, LOTTO_CLOSE.height});
    close.setPosition(LOTTO_CLOSE.left, LOTTO_CLOSE.top);
    close.setFillColor(sf::Color(150, 60, 60));
    close.setOutlineColor(sf::Color::White);
    close.setOutlineThickness(2.0f);
    window.draw(close);
    sf::Text x;
    x.setFont(*font_);
    x.setString("X");
    x.setCharacterSize(22);
    x.setFillColor(sf::Color::White);
    x.setPosition(LOTTO_CLOSE.left + 10.0f, LOTTO_CLOSE.top + 3.0f);
    window.draw(x);
}

HudPanel::LotteryHit HudPanel::lotteryHit(float mx, float my) const {
    if (LOTTO_REDEEM.contains(mx, my)) return LotteryHit::REDEEM;
    if (LOTTO_CLOSE.contains(mx, my))  return LotteryHit::CLOSE;
    return LotteryHit::NONE;
}

void HudPanel::renderMythicPreview(sf::RenderWindow& window, float mx, float my,
                                   MythicType m) const {
    if (!font_) return;

    const char* recipe = "";
    const char* line1  = "";
    const char* line2  = "";
    switch (m) {
        case MythicType::PHOENIX_BARRAGE:
            recipe = "DoubleShot + FireTrail";
            line1  = "Kill-urile pe inamici care ard dau embers.";
            line2  = "La 10 embers: furtuna de foc aprinde TOATA harta.";
            break;
        case MythicType::ROVING_BRUISER:
            recipe = "Knockback + Movable";
            line1  = "Knockback la a 2-a lovitura, arunca 2 celule,";
            line2  = "unda de soc AoE + vulnerabilitate +25% (3s).";
            break;
        case MythicType::SHIELDED_RUNNER:
            recipe = "ReflectShield + Movable";
            line1  = "Contact cu scutul PLIN: sarjeaza spre spawn,";
            line2  = "20% din HP curent + stun 1.5s; scutul se goleste.";
            break;
    }

    constexpr float W = 420.0f;
    constexpr float H = 118.0f;
    float x = std::min(mx + 24.0f, static_cast<float>(ui::WIN_W) - W - 10.0f);
    float y = std::min(my + 16.0f, static_cast<float>(ui::WIN_H) - H - 10.0f);

    sf::RectangleShape bg({W, H});
    bg.setPosition(x, y);
    bg.setFillColor(sf::Color(25, 18, 28, 245));
    bg.setOutlineColor(palette::rarityColor(Evolution::Rarity::MYTHIC));
    bg.setOutlineThickness(3.0f);
    window.draw(bg);

    auto line = [&](const std::string& s, unsigned size, float dy, sf::Color col) {
        sf::Text t;
        t.setFont(*font_);
        t.setString(s);
        t.setCharacterSize(size);
        t.setFillColor(col);
        t.setPosition(x + 12.0f, y + dy);
        window.draw(t);
    };
    line(mythicTypeName(m), 22, 8.0f,  palette::rarityColor(Evolution::Rarity::MYTHIC));
    line(recipe,            14, 38.0f, sf::Color(180, 160, 190));
    line(line1,             15, 62.0f, sf::Color(220, 220, 230));
    line(line2,             15, 84.0f, sf::Color(220, 220, 230));
}

void HudPanel::renderTokenPopup(sf::RenderWindow& window, const ShopPanel& shop,
                                float mx, float my) const {
    if (!font_) return;
    const auto& tokens = shop.getTokens();
    for (size_t i = 0; i < std::min<size_t>(tokens.size(), 3); ++i) {
        auto r = tokenSlotRect(static_cast<int>(i));
        if (mx < r.left || mx > r.left + r.width ||
            my < r.top  || my > r.top + r.height) continue;

        const auto& tk = tokens[i];
        // Descrierea REALA a efectului (cu numere), nu doar numele enum-ului.
        auto desc = tk.is_wildcard
            ? wrapText("Universal: se aplica pe un turn care are DEJA 2 Legendare "
                       "compatibile (o reteta) si devine evolutia Mythic.", 38)
            : wrapText(abilityDescription(tk.ability), 38);

        float pop_w = 310.0f;
        float pop_h = 34.0f + static_cast<float>(desc.size()) * 16.0f + 26.0f;
        sf::RectangleShape pop(sf::Vector2f(pop_w, pop_h));
        pop.setPosition(r.left + r.width + 8.0f, r.top);
        pop.setFillColor(sf::Color(25, 28, 40, 230));
        pop.setOutlineColor(palette::rarityColor(tk.rarity));
        pop.setOutlineThickness(2.0f);
        window.draw(pop);

        sf::Text t;
        t.setFont(*font_);
        t.setFillColor(sf::Color(220, 230, 255));
        t.setCharacterSize(18);
        t.setString(tk.name);
        t.setPosition(r.left + r.width + 16.0f, r.top + 6.0f);
        window.draw(t);

        t.setCharacterSize(13);
        float dy = r.top + 32.0f;
        for (const auto& l : desc) {
            t.setString(l);
            t.setPosition(r.left + r.width + 16.0f, dy);
            window.draw(t);
            dy += 16.0f;
        }

        t.setCharacterSize(14);
        t.setString("Click to apply on a tower");
        t.setFillColor(sf::Color(160, 200, 160));
        t.setPosition(r.left + r.width + 16.0f, dy + 4.0f);
        window.draw(t);
        return;
    }
}
