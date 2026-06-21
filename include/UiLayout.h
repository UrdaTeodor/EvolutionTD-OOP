#pragma once
#include <SFML/System/Vector2.hpp>

// Geometria ecranului si a grid-ului, partajata de toate layerele de randare
// (BoardRenderer, EffectsLayer, HudPanel, GameScene). O singura sursa de adevar:
// daca schimbam rezolutia sau marimea grid-ului, se schimba doar aici.
namespace ui {

    constexpr unsigned WIN_W   = 1920;
    constexpr unsigned WIN_H   = 1080;
    constexpr unsigned GRID_PX = 1080;
    constexpr unsigned GRID_X  = (WIN_W - GRID_PX) / 2;
    constexpr int      CELLS   = 20;
    constexpr float    CELL_PX = static_cast<float>(GRID_PX) / CELLS;

    // Coltul stanga-sus al celulei (row, col) in pixeli.
    inline sf::Vector2f cellToPx(int row, int col) {
        return { GRID_X + col * CELL_PX, row * CELL_PX };
    }

    // Centrul celulei in pixeli. Accepta coordonate fractionale (pozitii de inamici).
    inline sf::Vector2f cellCenter(float colF, float rowF) {
        return { GRID_X + (colF + 0.5f) * CELL_PX, (rowF + 0.5f) * CELL_PX };
    }

    // Conversie inversa: pixel -> celula (poate iesi din grid, verifica cu inGrid).
    inline int pxToCol(float mx) { return static_cast<int>((mx - GRID_X) / CELL_PX); }
    inline int pxToRow(float my) { return static_cast<int>(my / CELL_PX); }

    inline bool inGrid(int col, int row) {
        return col >= 0 && col < CELLS && row >= 0 && row < CELLS;
    }

} // namespace ui
