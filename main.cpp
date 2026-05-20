
//
// Turnuri disponibile (selectabile prin butoanele din panou dreapta):
//   1 = PrimitiveAV     - $50, off-path, medium damage, 1 shot/s, range 4
//   2 = Adblocker       - $40, off-path, low damage, 4 shots/s, range 3
//   3 = Honeypot        - $30, off-path, slows enemies 20% in range 1.5
//   4 = Firewall        - $60, on-path, absorbs enemies cu HP <= HP propriu
//   5 = BytecoinMiner   - $50, off-path, nu ataca, +25 credits/val (ROI 2 valuri)

//   Hover pe tower plasat = arata range. SPACE = porneste val. U = undo. ESC = iesi.
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>      // std::getenv
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include "Game.h"
#include "Wave.h"
#include "Enemy.h"
#include "GameException.h"
#include "EvolutionFactory.h"
#include "AbilityEvolution.h"
#include "AntivirusTower.h"

namespace {


constexpr unsigned WIN_W      = 1920;
constexpr unsigned WIN_H      = 1080;
constexpr unsigned GRID_PX    = 1080;
constexpr unsigned GRID_X     = (WIN_W - GRID_PX) / 2;
constexpr int      CELLS      = 20;
constexpr float    CELL_PX    = static_cast<float>(GRID_PX) / CELLS;

sf::Vector2f cellToPx(int row, int col) {
    return { GRID_X + col * CELL_PX, row * CELL_PX };
}
sf::Vector2f cellCenter(float colF, float rowF) {
    return { GRID_X + (colF + 0.5f) * CELL_PX, (rowF + 0.5f) * CELL_PX };
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

// Replica logica din Tower::calculateInterceptPoint pentru proiectile accurate vizual
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
        else if (t1 > 0.0f)               t = t1;
        else if (t2 > 0.0f)               t = t2;
        else return {ex, ey};
    }
    return {ex + vx * t, ey + vy * t};
}

std::vector<std::pair<int,int>> pathCells(const std::vector<std::pair<int,int>>& waypoints) {
    std::vector<std::pair<int,int>> cells;
    for (size_t i = 0; i + 1 < waypoints.size(); i++) {
        int r1 = waypoints[i].first,  c1 = waypoints[i].second;
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

//incarca PNG-uri din assets/sprites/, fallback la cercuri daca lipsesc.
struct Sprites {
    sf::Texture towerTex[5];
    bool        towerOk[5] = {false, false, false, false, false};
    sf::Texture enemyTex[4];   // 0=adware, 1=trojan, 2=worm, 3=ILOVEYOU (fallback single)
    bool        enemyOk[4] = {false, false, false, false};

    // Boss phase-uri
    // p1 = 100-80% HP, p2 = 80-60%, p3 = 60-40%, p4 = 40-20%, p5 = 20-0%
    sf::Texture bossPhases[5];
    bool        bossPhaseOk[5] = {false, false, false, false, false};

    void load() {
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
    }

    static int towerIndex(char c) {
        switch (c) {
            case 'A': return 0;
            case 'D': return 1;
            case 'H': return 2;
            case 'F': return 3;
            case 'M': return 4;
            default:  return -1;
        }
    }
    static int enemyIndex(const std::string& name) {
        if (name == "Adware")   return 0;
        if (name == "Trojan")   return 1;
        if (name == "Worm")     return 2;
        if (name == "ILOVEYOU") return 3;
        return -1;
    }

    static float displayScale(const std::string& name) {
        if (name == "ILOVEYOU") return 2.0f;   // boss
        if (name == "Trojan")   return 1.4f; 
        if (name == "Worm")     return 0.8f;
        return 1.0f;
    }


    static int bossPhaseForRatio(float ratio) {
        if (ratio > 0.8f) return 0;
        if (ratio > 0.6f) return 1;
        if (ratio > 0.4f) return 2;
        if (ratio > 0.2f) return 3;
        return 4;
    }

    // Returneaza textura potrivita
    // nullptr daca nu am sprite si trebuie fallback la cerc.
    const sf::Texture* textureFor(const Enemy& e) const {
        const std::string& name = e.getName();
        if (name == "ILOVEYOU") {
            float ratio = (e.getMaxHealth() > 0.0f) ? (e.getCurrentHealth() / e.getMaxHealth()) : 0.0f;
            int phase = bossPhaseForRatio(ratio);
            if (bossPhaseOk[phase]) return &bossPhases[phase];
            // fallback la sprite-ul single de boss
            if (enemyOk[3]) return &enemyTex[3];
            return nullptr;
        }
        int idx = enemyIndex(name);
        if (idx >= 0 && enemyOk[idx]) return &enemyTex[idx];
        return nullptr;
    }
};

struct Button {
    sf::RectangleShape rect;
    sf::Text           label;
    int                actionId;
    bool contains(float mx, float my) const {
        return rect.getGlobalBounds().contains(mx, my);
    }
};

Button makeTowerButton(int type, const char* text, float y, const sf::Font& font) {
    Button b;
    b.actionId = type;
    b.rect.setSize(sf::Vector2f(380.0f, 90.0f));
    b.rect.setPosition(GRID_X + GRID_PX + 20.0f, y);
    b.rect.setFillColor(towerColor("ADHFM"[type - 1]));
    b.rect.setOutlineColor(sf::Color::Black);
    b.rect.setOutlineThickness(2.0f);

    b.label.setFont(font);
    b.label.setString(text);
    b.label.setCharacterSize(22);
    b.label.setFillColor(sf::Color::Black);
    b.label.setPosition(GRID_X + GRID_PX + 40.0f, y + 28.0f);
    return b;
}

struct Projectile {
    sf::Vector2f start;
    sf::Vector2f end;
    float        progress;
    float        duration;
    sf::Color    color;
};

void drawRangeCircle(sf::RenderWindow& window, sf::Vector2f center, float radiusPx,
                     sf::Color fill, sf::Color outline) {
    sf::CircleShape c(radiusPx);
    c.setFillColor(fill);
    c.setOutlineColor(outline);
    c.setOutlineThickness(2.0f);
    c.setPosition(center.x - radiusPx, center.y - radiusPx);
    window.draw(c);
}


// Ruleaza inainte sa deschidem fereastra SFML. Output-ul ramane in consola
// si poate fi citit dupa ce inchizi jocul cu ESC.
void runT2DemoTests() {
    // Regula celor 3 pentru Wave (cc + op= + dtor implicit corect)
    {
        Wave w1(0, {});
        w1.addEnemy(makeAdware());

        Wave w2 = w1;
        w2.addEnemy(makeTrojan());

        Wave w3(0, {});
        w3 = w1;
        w3.addEnemy(makeWorm());

        std::cout << "=== Test Regula celor 3 ===\n";
        std::cout << "w1 original (1 inamic):         " << w1;
        std::cout << "w2 copie cc (2 inamici):        " << w2;
        std::cout << "w3 copie op= (2 inamici):       " << w3;
        std::cout << "=> w1 are tot 1 inamic: copiile sunt independente\n";
    }

    // Ierarhia proprie de exceptii (3 derivate prinse prin baza GameException)
    std::cout << "=== Test excepții (GameException + 3 derivate) ===\n";

    try {
        auto a = std::make_unique<AbilityEvolution>(
            "DoubleShot", 200, Evolution::Rarity::LEGENDARY,
            AbilityEvolution::AbilityType::DOUBLE_SHOT);
        auto b = std::make_unique<AbilityEvolution>(
            "ReflectShield", 200, Evolution::Rarity::LEGENDARY,
            AbilityEvolution::AbilityType::REFLECTIVE_SHIELD);
        craftMythic(std::move(a), std::move(b));
        std::cout << "  [BUG] throw\n";
    } catch (const IncompatibleEvolutionException& err) {
        std::cout << "  [ok] prins IncompatibleEvolutionException: " << err.what() << "\n";
    }

    try {
        AntivirusTower anti(0, 0);
        AbilityEvolution wrong("EpicBiggerAura", 100, Evolution::Rarity::EPIC,
                               AbilityEvolution::AbilityType::BIGGER_AURA);
        wrong.apply(anti);
        std::cout << "  [BUG] throw\n";
    } catch (const GameException& err) {
        std::cout << "  [ok] prins GameException: " << err.what() << "\n";
    }
}

} 

int main() {
    runT2DemoTests();

    std::cout << "=== EvolutionTD: Digital Immune System (SFML) ===\n";

    sf::RenderWindow window(sf::VideoMode(WIN_W, WIN_H), "EvolutionTD", sf::Style::Fullscreen);
    window.setFramerateLimit(60);

    sf::Font font;
    bool hasFont = loadFont(font);
    if (!hasFont) std::cerr << "Nu am gasit niciun font.\n";

    Sprites sprites;
    sprites.load();

    Game game;

    sf::RectangleShape uiLeft(sf::Vector2f(GRID_X, WIN_H));
    uiLeft.setFillColor(sf::Color(40, 40, 55));
    sf::RectangleShape uiRight(sf::Vector2f(GRID_X, WIN_H));
    uiRight.setPosition(GRID_X + GRID_PX, 0);
    uiRight.setFillColor(sf::Color(40, 40, 55));

    auto cells = pathCells(game.getPath());

    // per-cell quad cu (ii da un gradient subtil random)
    bool isOnPath[CELLS][CELLS] = {};
    for (const auto& [r, c] : cells) isOnPath[r][c] = true;
    
    // Vertex array pentru grid (aici ajutor cu AI)
    sf::VertexArray gridQuads(sf::Quads);
    {
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
                int amp = isOnPath[r][c] ? 8 : 12;
                float x = GRID_X + c * CELL_PX;
                float y = r * CELL_PX;
                gridQuads.append(sf::Vertex({x,           y},           vary(base, amp)));
                gridQuads.append(sf::Vertex({x + CELL_PX, y},           vary(base, amp)));
                gridQuads.append(sf::Vertex({x + CELL_PX, y + CELL_PX}, vary(base, amp)));
                gridQuads.append(sf::Vertex({x,           y + CELL_PX}, vary(base, amp)));
            }
        }
    }

    sf::VertexArray gridLines(sf::Lines);
    for (int i = 0; i <= CELLS; ++i) {
        gridLines.append(sf::Vertex(sf::Vector2f(GRID_X + i * CELL_PX, 0),       sf::Color(80, 200, 220, 25)));
        gridLines.append(sf::Vertex(sf::Vector2f(GRID_X + i * CELL_PX, GRID_PX), sf::Color(80, 200, 220, 25)));
        gridLines.append(sf::Vertex(sf::Vector2f(GRID_X,           i * CELL_PX), sf::Color(80, 200, 220, 25)));
        gridLines.append(sf::Vertex(sf::Vector2f(GRID_X + GRID_PX, i * CELL_PX), sf::Color(80, 200, 220, 25)));
    }

    std::vector<Button> towerButtons;
    if (hasFont) {
        towerButtons.push_back(makeTowerButton(1, "1. Antivirus  $50",       60.0f, font));
        towerButtons.push_back(makeTowerButton(2, "2. Adblocker  $40",      170.0f, font));
        towerButtons.push_back(makeTowerButton(3, "3. Honeypot   $30",      280.0f, font));
        towerButtons.push_back(makeTowerButton(4, "4. Firewall   $60 (path)", 390.0f, font));
        towerButtons.push_back(makeTowerButton(5, "5. Miner      $50",      500.0f, font));
    }

    sf::Text hpText, moneyText, waveText, statusText, lastErrText;
    auto setupText = [&](sf::Text& t, unsigned size, float y, sf::Color col) {
        t.setFont(font);
        t.setCharacterSize(size);
        t.setFillColor(col);
        t.setPosition(20.0f, y);
    };
    if (hasFont) {
        setupText(hpText,      36, 60.0f,  sf::Color(220, 220, 220));
        setupText(moneyText,   36, 110.0f, sf::Color(220, 220, 100));
        setupText(waveText,    36, 160.0f, sf::Color(220, 220, 220));
        setupText(statusText,  24, 230.0f, sf::Color(180, 220, 180));
        setupText(lastErrText, 20, 1010.0f, sf::Color(230, 100, 100));
    }

    sf::Clock clock;
    sf::Clock gameClock;
    bool waveRunning = false;
    int  selectedType = 0;
    std::string lastError;
    sf::Clock errClock;

    std::vector<float>      lastFireTime;
    std::vector<Projectile> projectiles;

    // Boss kill = victorie automata (val 5). Cand ILOVEYOU moare, ignoram worms-urile ramase.
    bool bossWasActive  = false;
    bool bossKillWin    = false;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;

        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) window.close();
                if (event.key.code == sf::Keyboard::Space &&
                    !waveRunning && !game.allWavesDone() && !game.isGameOver() && !bossKillWin) {
                    game.startWave();
                    waveRunning = true;
                }
                if (event.key.code == sf::Keyboard::U) {
                    if (game.restoreSnapshot()) {
                        lastError = ">>> Undo: revenit la wavestart " + std::to_string(game.getWaveNumber());
                        errClock.restart();
                        
                        waveRunning    = false;
                        bossWasActive  = false;
                        bossKillWin    = false;
                        projectiles.clear();
                        lastFireTime.assign(game.getTowers().size(), gameClock.getElapsedTime().asSeconds());
                    }
                }
            }

            if (event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left) {
                float mx = static_cast<float>(event.mouseButton.x);
                float my = static_cast<float>(event.mouseButton.y);

                bool clickedButton = false;
                for (const auto& btn : towerButtons) {
                    if (btn.contains(mx, my)) {
                        selectedType = btn.actionId;
                        clickedButton = true;
                        break;
                    }
                }

                if (!clickedButton && selectedType != 0) {
                    int col = static_cast<int>((mx - GRID_X) / CELL_PX);
                    int row = static_cast<int>(my / CELL_PX);
                    if (col >= 0 && col < CELLS && row >= 0 && row < CELLS) {
                        try {
                            game.placeTower(selectedType, col, row);
                        } catch (const GameException& err) {
                            lastError = err.what();
                            errClock.restart();
                        }
                    }
                }
            }
        }

        while (lastFireTime.size() < game.getTowers().size())
            lastFireTime.push_back(gameClock.getElapsedTime().asSeconds());

        if (waveRunning) {
            game.tickWave(dt);
            if (!game.isWaveActive()) {
                game.endWave();
                waveRunning = false;
            }

            // Boss kill detection (doar val 5)
            //victorie automata si oprim valul ignorand worm-urile ramase.
            if (game.getWaveNumber() == 5 || bossWasActive) {
                bool bossNow = false;
                for (const auto& e : game.getCurrentWave().getActiveEnemies()) {
                    if (e.isAlive() && e.getName() == "ILOVEYOU") { bossNow = true; break; }
                }
                if (bossNow) bossWasActive = true;
                if (bossWasActive && !bossNow && !bossKillWin) {
                    bossKillWin  = true;
                    waveRunning  = false;
                }
            }
        }
        // Logica de atac a turnurilor (doar pentru alea care trag proiectile, adica Antivirus si Adblocker)
        if (waveRunning) {
            float now = gameClock.getElapsedTime().asSeconds();
            const auto& towers  = game.getTowers();
            const auto& enemies = game.getCurrentWave().getActiveEnemies();
            for (size_t i = 0; i < towers.size(); i++) {
                char ch = towers[i]->getDisplayChar();
                float interval = fireIntervalForChar(ch);
                if (interval <= 0.0f) continue;
                if (now - lastFireTime[i] < interval) continue;

                float range = towers[i]->getRange();
                float bestDist = 1e9f;
                const Enemy* nearest = nullptr;
                for (const auto& e : enemies) {
                    if (!e.isAlive()) continue;
                    float dx = e.getX() - static_cast<float>(towers[i]->getX());
                    float dy = e.getY() - static_cast<float>(towers[i]->getY());
                    float d = std::sqrt(dx*dx + dy*dy);
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
                    float ddx = ix - static_cast<float>(towers[i]->getX());
                    float ddy = iy - static_cast<float>(towers[i]->getY());
                    float dist = std::sqrt(ddx * ddx + ddy * ddy);
                    p.duration = (ps > 0.0f) ? (dist / ps) : 0.3f;
                    if (p.duration < 0.05f) p.duration = 0.05f;
                    p.color    = (ch == 'A') ? sf::Color(150, 220, 255) : sf::Color(255, 240, 100);
                    projectiles.push_back(p);
                    lastFireTime[i] = now;
                }
            }
        }

        for (auto& p : projectiles) p.progress += dt / p.duration;
        projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
            [](const Projectile& p) { return p.progress >= 1.0f; }), projectiles.end());

        window.clear(sf::Color(20, 20, 30));
        window.draw(uiLeft);
        window.draw(uiRight);
        window.draw(gridQuads);
        window.draw(gridLines);

        sf::Vector2i mouseI = sf::Mouse::getPosition(window);
        float mx = static_cast<float>(mouseI.x);
        float my = static_cast<float>(mouseI.y);
        int hoverCol = static_cast<int>((mx - static_cast<float>(GRID_X)) / CELL_PX);
        int hoverRow = static_cast<int>(my / CELL_PX);
        bool inGrid = (hoverCol >= 0 && hoverCol < CELLS && hoverRow >= 0 && hoverRow < CELLS);

        const Tower* hoveredTower = nullptr;
        if (inGrid) {
            for (const auto& t : game.getTowers()) {
                if (t->getX() == hoverCol && t->getY() == hoverRow) {
                    hoveredTower = t.get();
                    break;
                }
            }
        }

        if (hoveredTower && hoveredTower->getRange() > 0.0f) {
            sf::Vector2f c = cellCenter(static_cast<float>(hoveredTower->getX()),
                                        static_cast<float>(hoveredTower->getY()));
            drawRangeCircle(window, c, hoveredTower->getRange() * CELL_PX,
                            sf::Color(255, 255, 255, 35),
                            sf::Color(255, 255, 255, 130));
        }
        
        if (!hoveredTower && inGrid && selectedType != 0) {
            float r = rangeForType(selectedType);
            if (r > 0.0f) {
                sf::Vector2f c = cellCenter(static_cast<float>(hoverCol),
                                            static_cast<float>(hoverRow));
                drawRangeCircle(window, c, r * CELL_PX,
                                sf::Color(255, 255, 255, 25),
                                sf::Color(255, 255, 255, 100));
            }
            sf::RectangleShape hl(sf::Vector2f(CELL_PX, CELL_PX));
            hl.setPosition(cellToPx(hoverRow, hoverCol));
            hl.setFillColor(sf::Color(255, 255, 255, 60));
            window.draw(hl);
        }

        for (const auto& tower : game.getTowers()) {
            int idx = Sprites::towerIndex(tower->getDisplayChar());
            sf::Vector2f c = cellCenter(static_cast<float>(tower->getX()),
                                        static_cast<float>(tower->getY()));
            if (idx >= 0 && sprites.towerOk[idx]) {
                sf::Sprite spr(sprites.towerTex[idx]);
                sf::Vector2u sz = sprites.towerTex[idx].getSize();
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

        for (const auto& enemy : game.getCurrentWave().getActiveEnemies()) {
            if (!enemy.isAlive()) continue;
            sf::Vector2f c = cellCenter(enemy.getX(), enemy.getY());
            float ds = Sprites::displayScale(enemy.getName());  // 1x normal, 2x boss
            const sf::Texture* tex = sprites.textureFor(enemy);  // alege phase-ul corect pt boss
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

        for (const auto& p : projectiles) {
            sf::Vector2f pos = p.start + (p.end - p.start) * p.progress;
            sf::CircleShape dot(6.0f);
            dot.setFillColor(p.color);
            dot.setOutlineColor(sf::Color::Black);
            dot.setOutlineThickness(1.0f);
            dot.setPosition(pos.x - 6.0f, pos.y - 6.0f);
            window.draw(dot);
        }

        for (const auto& btn : towerButtons) {
            window.draw(btn.rect);
            window.draw(btn.label);
            if (btn.actionId == selectedType) {
                sf::RectangleShape sel = btn.rect;
                sel.setFillColor(sf::Color::Transparent);
                sel.setOutlineColor(sf::Color::White);
                sel.setOutlineThickness(4.0f);
                window.draw(sel);
            }
        }

        if (hasFont) {
            hpText.setString("HP:     " + std::to_string(game.getPlayerHP()));
            moneyText.setString("$:      " + std::to_string(game.getMoney()));
            waveText.setString("Wave:   " + std::to_string(std::min(game.getWaveNumber(), Game::getMaxWaves()))
                                          + "/" + std::to_string(Game::getMaxWaves()));

            std::string status;
            if (game.isGameOver())         status = "GAME OVER (ESC)";
            else if (bossKillWin)          status = "VICTORY! ILOVEYOU eliminat (ESC)";
            else if (game.allWavesDone())  status = "VICTORY! (ESC)";
            else if (waveRunning)          status = "Wave in curs...";
            else if (selectedType != 0)    status = "Click pe grid sa plasezi";
            else                            status = "SPACE = start wave\nU = undo (intre valuri)\nClick pe un buton = selecteaza";
            statusText.setString(status);

            if (!lastError.empty() && errClock.getElapsedTime().asSeconds() < 3.0f) {
                lastErrText.setString(lastError);
                window.draw(lastErrText);
            }

            window.draw(hpText);
            window.draw(moneyText);
            window.draw(waveText);
            window.draw(statusText);
        }

        window.display();
    }

    return 0;
}
