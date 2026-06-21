#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "MythicType.h"

class Game;
class ShopPanel;
class Tower;
class GlobalStatBuffs;
class DataRegistry;

// Panourile laterale: butoane de tower, butonul de start/fast-forward, butonul
// de shop, textele HP/$/Wave/status, sloturile de token si panoul de info tower.
// HudPanel doar afiseaza si raspunde la "ce e sub mouse?" (hitTest);
// deciziile de gameplay raman in GameScene.
class HudPanel {
public:
    // Rezultatul unui hit test: pe ce element de HUD a picat click-ul.
    struct Hit {
        enum class Kind { NONE, TOWER_BUTTON, FF, SHOP_TOGGLE, TOKEN_SLOT };
        Kind kind  = Kind::NONE;
        int  index = 0;   // TOWER_BUTTON: tipul (1-5); TOKEN_SLOT: indexul slotului
    };

    // Datele de afisat in frame-ul curent (calculate de GameScene).
    struct Frame {
        int selected_type = 0;
        int game_speed    = 0;
        std::string status;
        std::string error;        // gol => nu se afiseaza
        float mouse_x = 0.0f;
        float mouse_y = 0.0f;
    };

    HudPanel(const sf::Font* font, const DataRegistry& registry);

    Hit  hitTest(float mx, float my, int tokenCount) const;
    bool sellButtonContains(float mx, float my, const Tower& tower) const;
    // Butonul TARGET din info panel (doar la turnurile care trag).
    bool targetButtonContains(float mx, float my, const Tower& tower) const;
    // Butonul MOVE din info panel (doar la turnurile cu MOVABLE).
    bool moveButtonContains(float mx, float my, const Tower& tower) const;

    // Fundalul panourilor (sub grid), respectiv continutul (peste tabla de joc).
    void renderBackground(sf::RenderWindow& window) const;
    void renderFront(sf::RenderWindow& window, const Game& game,
                     const ShopPanel& shop, const Frame& frame);

    // mx/my: hover pe o linie de evolutie arata tooltip cu ce face efectiv.
    // Panoul isi calculeaza inaltimea dinamic din continut (toate evolutiile
    // sunt vizibile, grupate cu contor "xN").
    void renderTowerInfo(sf::RenderWindow& window, const Tower* tower,
                         const GlobalStatBuffs& buffs, float mx, float my) const;

    // Popup-ul de hover pe un slot de token. Desenat separat (dupa info panel),
    // inaintea overlay-ului de shop.
    void renderTokenPopup(sf::RenderWindow& window, const ShopPanel& shop,
                          float mx, float my) const;

    // Tinta monedelor de income (centrul contorului "$:"), pentru EffectsLayer.
    static sf::Vector2f moneyCounterPos() { return { 95.0f, 132.0f }; }

    // Popup-ul de loterie (scam-ad). Modal: desenat peste tot, raspuns obligatoriu.
    enum class LotteryHit { NONE, REDEEM, CLOSE };
    void renderLotteryPopup(sf::RenderWindow& window) const;
    LotteryHit lotteryHit(float mx, float my) const;

    // Preview-ul evolutiei Mythic (nume + descriere) langa mouse, cand
    // wildcard-ul e activ si hover-ul e pe un turn compatibil.
    void renderMythicPreview(sf::RenderWindow& window, float mx, float my,
                             MythicType m) const;

private:
    struct Button {
        sf::RectangleShape rect;
        sf::Text label;
        int actionId = 0;
        bool contains(float mx, float my) const {
            return rect.getGlobalBounds().contains(mx, my);
        }
    };

    const sf::Font* font_;   // nullptr daca fontul nu s-a incarcat

    sf::RectangleShape ui_left_;
    sf::RectangleShape ui_right_;
    std::vector<Button> tower_buttons_;
    Button ff_button_;
    Button shop_button_;
    sf::Text hp_text_;
    sf::Text money_text_;
    sf::Text wave_text_;
    sf::Text status_text_;
    sf::Text err_text_;

    // Count-up animat la bani: contorul alearga spre valoarea reala in loc sa
    // sara, si pulseaza colorat la schimbare (verde = incasare, rosu = plata).
    float     shown_money_ = -1.0f;   // -1 = neinitializat, snap la prima afisare
    int       last_money_  = 0;
    float     money_pulse_ = 0.0f;    // secunde ramase de puls
    bool      money_up_    = true;    // directia ultimei schimbari
    sf::Clock money_clock_;           // dt propriu (renderFront nu primeste dt)

    static sf::FloatRect tokenSlotRect(int i);
    // Coltul stanga-sus al panoului de info pentru un tower (stanga/dreapta lui).
    static sf::Vector2f infoPanelPos(const Tower& tower);
    // Inaltimea panoului, calculata din continut (stats + evolutii grupate +
    // butoane) — render-ul si hit-test-urile butoanelor folosesc aceeasi sursa.
    static float infoPanelHeight(const Tower& tower);
    static float moveButtonY(const Tower& tower);
    static float targetButtonY(const Tower& tower);
    static float sellButtonY(const Tower& tower);
};
