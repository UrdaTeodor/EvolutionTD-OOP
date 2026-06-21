#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <vector>
#include <utility>
#include "AbilityType.h"

class Game;
class Tower;
class Enemy;
class DataRegistry;
class GlobalStatBuffs;
class EffectsLayer;
struct Sprites;

// Randarea tablei de joc: fundalul grid-ului, turnurile, inamicii si overlay-urile
// de pe grid (cerc de range, preview de plasare, halo-uri pentru token apply).
// Nu detine stare de joc; primeste Game-ul const la fiecare frame.
class BoardRenderer {
public:
    explicit BoardRenderer(const std::vector<std::pair<int, int>>& waypoints);

    // Reconstruieste fundalul (apelat si dupa load de save, cand harta se poate schimba).
    void rebuild(const std::vector<std::pair<int, int>>& waypoints);

    void renderGrid(sf::RenderWindow& window) const;
    // effects: recoil-ul (turnul "sare" cand trage) + timpul pentru pulsatia
    // aurelor de investitie.
    void renderTowers(sf::RenderWindow& window, const Game& game, const Sprites& sprites,
                      const EffectsLayer& effects) const;
    void renderEnemies(sf::RenderWindow& window, const Game& game, const Sprites& sprites) const;

    // Cerc de range pentru un turn existent (hover).
    void renderTowerRange(sf::RenderWindow& window, const Tower& tower,
                          const GlobalStatBuffs& buffs) const;

    // Preview la plasare: highlight celula + cercul de range al tipului selectat.
    void renderPlacementPreview(sf::RenderWindow& window, const Game& game,
                                const DataRegistry& registry, int selectedType,
                                int hoverCol, int hoverRow) const;

    // Halo verde pe turnurile compatibile cu token-ul activ, rosu slab pe restul.
    // Compatibilitatea o decide caller-ul (difera intre token normal si Mythic
    // Token universal), de-aia primim un predicat.
    void renderTokenHalos(sf::RenderWindow& window, const Game& game,
                          const std::function<bool(const Tower&)>& compatible) const;

private:
    sf::VertexArray grid_quads_;
    sf::VertexArray grid_lines_;

    void renderBossBar(sf::RenderWindow& window, const Enemy& boss) const;

    static void drawRangeCircle(sf::RenderWindow& window, sf::Vector2f center,
                                float radiusPx, sf::Color fill, sf::Color outline);
};
