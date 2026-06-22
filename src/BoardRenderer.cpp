#include "BoardRenderer.h"
#include "Game.h"
#include "Tower.h"
#include "Enemy.h"
#include "HoneypotTower.h"
#include "Sprites.h"
#include "DataRegistry.h"
#include "EffectsLayer.h"
#include "GlobalStatBuffs.h"
#include "Palette.h"
#include "PathUtils.h"
#include "UiLayout.h"
#include <algorithm>
#include <cmath>
#include <random>

// disclaimer: Mult AI

namespace {

    std::vector<std::pair<int,int>> pathCells(const std::vector<std::pair<int,int>>& waypoints) {
        return expandPathCells(waypoints);
    }

    // Culoarea pip-ului de evolutie de pe turn, dupa raritatea abilitatii.
    sf::Color abilityPipColor(AbilityType ab) {
        switch (ab) {
            case AbilityType::MULTI_TARGET:
            case AbilityType::BIGGER_AURA:
            case AbilityType::ARMORED:
            case AbilityType::AMPLIFY:
                return palette::rarityColor(Evolution::Rarity::EPIC);
            default:
                return palette::rarityColor(Evolution::Rarity::LEGENDARY);
        }
    }

} // namespace

BoardRenderer::BoardRenderer(const std::vector<std::pair<int, int>>& waypoints)
    : grid_quads_(sf::Quads), grid_lines_(sf::Lines) {
    rebuild(waypoints);
}

// cu AI aici pentru gradientul de culori al celulelor.
void BoardRenderer::rebuild(const std::vector<std::pair<int, int>>& waypoints) {
    grid_quads_.clear();
    grid_lines_.clear();

    auto cells = pathCells(waypoints);

    bool isOnPath[ui::CELLS][ui::CELLS] = {};
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
    for (int r = 0; r < ui::CELLS; ++r) {
        for (int c = 0; c < ui::CELLS; ++c) {
            sf::Color base = isOnPath[r][c] ? pathBase : bgBase;
            int amp        = isOnPath[r][c] ? 8 : 12;
            float x = ui::GRID_X + c * ui::CELL_PX;
            float y = r * ui::CELL_PX;
            grid_quads_.append(sf::Vertex({x,               y},               vary(base, amp)));
            grid_quads_.append(sf::Vertex({x + ui::CELL_PX, y},               vary(base, amp)));
            grid_quads_.append(sf::Vertex({x + ui::CELL_PX, y + ui::CELL_PX}, vary(base, amp)));
            grid_quads_.append(sf::Vertex({x,               y + ui::CELL_PX}, vary(base, amp)));
        }
    }

    const sf::Color lineCol(80, 200, 220, 25);
    for (int i = 0; i <= ui::CELLS; ++i) {
        grid_lines_.append(sf::Vertex(sf::Vector2f(ui::GRID_X + i * ui::CELL_PX, 0),           lineCol));
        grid_lines_.append(sf::Vertex(sf::Vector2f(ui::GRID_X + i * ui::CELL_PX, ui::GRID_PX), lineCol));
        grid_lines_.append(sf::Vertex(sf::Vector2f(ui::GRID_X,               i * ui::CELL_PX), lineCol));
        grid_lines_.append(sf::Vertex(sf::Vector2f(ui::GRID_X + ui::GRID_PX, i * ui::CELL_PX), lineCol));
    }
}

void BoardRenderer::renderGrid(sf::RenderWindow& window) const {
    window.draw(grid_quads_);
    window.draw(grid_lines_);
}

void BoardRenderer::renderTowers(sf::RenderWindow& window, const Game& game,
                                 const Sprites& sprites,
                                 const EffectsLayer& effects) const {
    for (const auto& tower : game.getTowers()) {
        int idx = Sprites::towerIndex(tower->getDisplayChar());
        // visualPosition difera de celula logica in timpul sarjei ShieldedRunner.
        auto [vcol, vrow] = tower->visualPosition();
        sf::Vector2f c = ui::cellCenter(vcol, vrow);

        // Aura Honeypot vizibila permanent (slow + evolutiile de suport):
        // identitatea turnului E aura, nu sprite-ul. Inel cyan = Overclock
        // (atac mai rapid inauntru), inel magenta = Amplify (damage in plus).
        if (const auto* hp = dynamic_cast<const HoneypotTower*>(tower.get())) {
            float r = hp->auraRange(game.getBuffs()) * ui::CELL_PX;
            sf::CircleShape aura(r);
            aura.setPosition(c.x - r, c.y - r);
            aura.setFillColor(sf::Color(230, 150, 60, 14));
            aura.setOutlineThickness(1.5f);
            aura.setOutlineColor(sf::Color(230, 150, 60, 90));
            window.draw(aura);

            auto supportRing = [&](float radius, sf::Color col) {
                sf::CircleShape ring(radius);
                ring.setFillColor(sf::Color::Transparent);
                ring.setOutlineThickness(2.0f);
                ring.setOutlineColor(col);
                ring.setPosition(c.x - radius, c.y - radius);
                window.draw(ring);
            };
            float pulse = 2.5f * std::sin(effects.time() * 4.0f);
            if (hp->overclockStacks() > 0) {
                supportRing(r - 5.0f - pulse, sf::Color(80, 220, 255, 130));
            }
            if (hp->amplifyStacks() > 0) {
                supportRing(r - 11.0f + pulse, sf::Color(255, 90, 200, 130));
            }
        }

        // Aura de investitie: glow permanent sub turn, creste cu suma
        // token-urilor aplicate — build-ul se vede de la o privire.
        int inv = tower->getTokenInvestment();
        if (inv > 0) {
            float t     = std::min(1.0f, static_cast<float>(inv) / 900.0f);
            float pulse = 0.9f + 0.1f * std::sin(effects.time() * 2.5f
                                                 + tower->instanceId());
            sf::Sprite glow(sprites.glowTex);
            sf::Vector2u gs = sprites.glowTex.getSize();
            float size = ui::CELL_PX * (1.3f + 1.1f * t) * pulse;
            glow.setOrigin(gs.x / 2.0f, gs.y / 2.0f);
            glow.setScale(size / gs.x, size / gs.y);
            glow.setPosition(c);
            glow.setColor(sf::Color(255, 200, 110,
                          static_cast<sf::Uint8>(45.0f + 75.0f * t)));
            window.draw(glow, sf::RenderStates(sf::BlendAdd));
        }

        // Recoil: turnul se umfla scurt cand trage (squash & stretch).
        float kick  = effects.towerRecoil(tower->instanceId());
        float scale = 1.0f + 0.16f * kick;

        if (idx >= 0 && sprites.towerOk[idx]) {
            sf::Sprite spr(sprites.towerTex[idx]);
            sf::Vector2u sz = sprites.towerTex[idx].getSize();
            spr.setOrigin(sz.x / 2.0f, sz.y / 2.0f);
            spr.setScale(scale * ui::CELL_PX / sz.x, scale * ui::CELL_PX / sz.y);
            spr.setPosition(c);
            window.draw(spr);
        } else {
            float radius = ui::CELL_PX * 0.35f * scale;
            sf::CircleShape t(radius);
            t.setFillColor(palette::towerColor(tower->getDisplayChar()));
            t.setOutlineColor(sf::Color::Black);
            t.setOutlineThickness(2.0f);
            t.setPosition(c.x - radius, c.y - radius);
            window.draw(t);
        }

        // Build-ul se vede pe tabla: inel rosu = Mythic, pip-uri colorate =
        // abilitatile aplicate (mov epic / portocaliu legendar).
        if (tower->mythicBadge()) {
            float rr = ui::CELL_PX * 0.48f;
            sf::CircleShape ring(rr);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineColor(palette::rarityColor(Evolution::Rarity::MYTHIC));
            ring.setOutlineThickness(3.0f);
            ring.setPosition(c.x - rr, c.y - rr);
            window.draw(ring);
        }
        auto abilities = tower->getAppliedAbilities();
        int shown = std::min<int>(static_cast<int>(abilities.size()), 6);
        for (int i = 0; i < shown; ++i) {
            sf::RectangleShape pip({6.0f, 6.0f});
            pip.setFillColor(abilityPipColor(abilities[i]));
            pip.setOutlineColor(sf::Color::Black);
            pip.setOutlineThickness(1.0f);
            pip.setPosition(c.x - ui::CELL_PX * 0.45f + i * 9.0f,
                            c.y + ui::CELL_PX * 0.34f);
            window.draw(pip);
        }
    }
}

void BoardRenderer::renderEnemies(sf::RenderWindow& window, const Game& game,
                                  const Sprites& sprites) const {
    const Enemy* boss = nullptr;

    for (const auto& enemy : game.getCurrentWave().getActiveEnemies()) {
        if (!enemy.isAlive()) continue;
        sf::Vector2f c = ui::cellCenter(enemy.getX(), enemy.getY());
        float ds = Sprites::displayScale(enemy.getName());
        const sf::Texture* tex = sprites.textureFor(enemy);
        // Slow (Honeypot / exces de knockback): tenta albastra pe sprite —
        // incetinirea se vede direct pe inamic, nu doar in viteza de mers.
        sf::Color tint = enemy.isSlowed() ? sf::Color(130, 170, 255)
                                          : sf::Color::White;
        if (tex) {
            sf::Sprite spr(*tex);
            sf::Vector2u sz = tex->getSize();
            spr.setOrigin(sz.x / 2.0f, sz.y / 2.0f);
            spr.setScale(ds * ui::CELL_PX / sz.x, ds * ui::CELL_PX / sz.y);
            spr.setPosition(c);
            spr.setColor(tint);
            window.draw(spr);
        } else {
            float radius = ui::CELL_PX * 0.25f * ds;
            sf::CircleShape e(radius);
            e.setFillColor(enemy.isSlowed() ? sf::Color(120, 90, 200)
                                            : sf::Color(220, 50, 50));
            e.setOutlineColor(sf::Color::Black);
            e.setOutlineThickness(1.5f);
            e.setPosition(c.x - radius, c.y - radius);
            window.draw(e);
        }

        if (enemy.getName() == "ILOVEYOU") boss = &enemy;

        // Health bar mic deasupra inamicului, doar daca a fost lovit.
        float ratio = (enemy.getMaxHealth() > 0.0f)
                          ? enemy.getCurrentHealth() / enemy.getMaxHealth()
                          : 0.0f;
        if (ratio < 1.0f) {
            float bw = ui::CELL_PX * 0.8f * ds;
            float bh = 5.0f;
            float bx = c.x - bw / 2.0f;
            float by = c.y - ds * ui::CELL_PX * 0.5f - 9.0f;

            sf::RectangleShape back({bw, bh});
            back.setPosition(bx, by);
            back.setFillColor(sf::Color(20, 20, 20, 200));
            window.draw(back);

            sf::Color col = (ratio > 0.5f)  ? sf::Color(90, 220, 90)
                          : (ratio > 0.25f) ? sf::Color(230, 200, 60)
                                            : sf::Color(230, 70, 60);
            sf::RectangleShape fill({bw * ratio, bh});
            fill.setPosition(bx, by);
            fill.setFillColor(col);
            window.draw(fill);
        }
    }

    if (boss) renderBossBar(window, *boss);
}

// Bara mare de HP a boss-ului, sus peste grid, cu gradatii la fiecare 20%
// (pragurile la care boss-ul isi schimba faza/sprite-ul).
// cppcheck-suppress functionStatic
void BoardRenderer::renderBossBar(sf::RenderWindow& window, const Enemy& boss) const {
    float ratio = (boss.getMaxHealth() > 0.0f)
                      ? boss.getCurrentHealth() / boss.getMaxHealth()
                      : 0.0f;

    const float bw = ui::GRID_PX - 80.0f;
    const float bh = 22.0f;
    const float bx = ui::GRID_X + 40.0f;
    const float by = 12.0f;

    sf::RectangleShape back({bw, bh});
    back.setPosition(bx, by);
    back.setFillColor(sf::Color(15, 15, 25, 220));
    back.setOutlineColor(sf::Color(220, 60, 60));
    back.setOutlineThickness(2.0f);
    window.draw(back);

    sf::RectangleShape fill({bw * ratio, bh});
    fill.setPosition(bx, by);
    fill.setFillColor(sf::Color(200, 40, 70));
    window.draw(fill);

    for (int i = 1; i < 5; ++i) {
        sf::RectangleShape tick({2.0f, bh});
        tick.setPosition(bx + bw * 0.2f * i - 1.0f, by);
        tick.setFillColor(sf::Color(15, 15, 25, 220));
        window.draw(tick);
    }
}

// cppcheck-suppress functionStatic
void BoardRenderer::renderTowerRange(sf::RenderWindow& window, const Tower& tower,
                                     const GlobalStatBuffs& buffs) const {
    if (tower.getRange() <= 0.0f) return;
    // Honeypot: cercul de hover arata raza REALA a aurei (cu BiggerAura),
    // nu range-ul de baza — altfel hover-ul contrazicea aura desenata.
    float range = tower.effectiveRange(buffs);
    if (const auto* hp = dynamic_cast<const HoneypotTower*>(&tower)) {
        range = hp->auraRange(buffs);
    }
    sf::Vector2f c = ui::cellCenter(static_cast<float>(tower.getX()),
                                    static_cast<float>(tower.getY()));
    drawRangeCircle(window, c, range * ui::CELL_PX,
                    sf::Color(255, 255, 255, 35),
                    sf::Color(255, 255, 255, 130));
}

// cppcheck-suppress functionStatic
void BoardRenderer::renderPlacementPreview(sf::RenderWindow& window, const Game& game,
                                           const DataRegistry& registry, int selectedType,
                                           int hoverCol, int hoverRow) const {
    static const char* keys[] = {"antivirus","adblocker","honeypot","firewall","bytecoinminer"};
    if (selectedType < 1 || selectedType > 5) return;
    const std::string& key = keys[selectedType - 1];
    float baseR = registry.getTower(key).range;
    if (baseR > 0.0f) {
        float effR = baseR * (1.0f + game.getBuffs().pct(key, "range_pct"));
        sf::Vector2f c = ui::cellCenter(static_cast<float>(hoverCol),
                                        static_cast<float>(hoverRow));
        drawRangeCircle(window, c, effR * ui::CELL_PX,
                        sf::Color(255, 255, 255, 25),
                        sf::Color(255, 255, 255, 100));
    }
    sf::RectangleShape hl(sf::Vector2f(ui::CELL_PX, ui::CELL_PX));
    hl.setPosition(ui::cellToPx(hoverRow, hoverCol));
    hl.setFillColor(sf::Color(255, 255, 255, 60));
    window.draw(hl);
}

// cppcheck-suppress functionStatic
void BoardRenderer::renderTokenHalos(sf::RenderWindow& window, const Game& game,
                                     const std::function<bool(const Tower&)>& compatible) const {
    for (const auto& t : game.getTowers()) {
        sf::Vector2f c = ui::cellCenter(static_cast<float>(t->getX()),
                                        static_cast<float>(t->getY()));
        if (compatible(*t)) {
            drawRangeCircle(window, c, ui::CELL_PX * 0.55f,
                            sf::Color( 80, 255,  80,  70),
                            sf::Color( 80, 255,  80, 220));
        } else {
            drawRangeCircle(window, c, ui::CELL_PX * 0.45f,
                            sf::Color(150,  60,  60,  40),
                            sf::Color(150,  60,  60, 120));
        }
    }
}

void BoardRenderer::drawRangeCircle(sf::RenderWindow& window, sf::Vector2f center,
                                    float radiusPx, sf::Color fill, sf::Color outline) {
    sf::CircleShape c(radiusPx);
    c.setFillColor(fill);
    c.setOutlineColor(outline);
    c.setOutlineThickness(2.0f);
    c.setPosition(center.x - radiusPx, center.y - radiusPx);
    window.draw(c);
}
