#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <string>
#include <random>
#include "EvolutionFactory.h"
#include "EvolutionSpecs.h"
#include "EvolutionToken.h"

class GlobalStatBuffs;
class DataRegistry;
struct SaveData;

class ShopPanel {
public:
    enum class ClickResult {
        NONE,               // click ignorat 
        CLOSE,              
        BUY_FAILED,        
        BUY_OK_INSTANT,     
        BUY_OK_TOKEN,       
        TOKEN_PICKED        
    };

    ShopPanel(GlobalStatBuffs& buffs, const DataRegistry& registry);


    void loadFromJson(const std::string& path);
    void refresh(std::mt19937& rng, bool offer_major);

    // Asociere font 
    void setFont(const sf::Font& font);

    // Open / Close shop overlay.
    void toggle()             { visible_ = !visible_; }
    bool visible() const      { return visible_; }
    void setVisible(bool v)   { visible_ = v; }

    // Render overlay
    void render(sf::RenderWindow& window, int money) const;

    // Tratare click. Modifica `money` + `player_weight` direct  la cumparare reusita.
    ClickResult handleClick(float mx, float my, int& money, int& player_weight, std::string& outErr);

    //Token apply mode (folosit de GameScene)
    bool hasActiveToken() const { return active_token_idx_ >= 0; }
    const EvolutionToken* activeToken() const;
    int  activeTokenIndex() const { return active_token_idx_; }
    void setActiveTokenIndex(int idx) { active_token_idx_ = idx; }
    const std::vector<EvolutionToken>& getTokens() const { return tokens_; }

    // Scoate token-ul activ din inventar (apelat dupa apply reusit pe tower).
    void consumeActiveToken();

    // Anuleaza mode-ul apply fara consum (apelat la click in afara unui tower compatibil).
    void cancelActiveToken();

    // Folosit pentru label butonului "OPEN SHOP" in left panel.
    int tokenCount() const { return static_cast<int>(tokens_.size()); }

    // Total carduri active in offer (pt status / debug).
    int totalOfferedCards() const;

    // Save / Load — scrie in mini_offer/major_offer/tokens campuri din SaveData / restaureaza din ele.
    void serializeTo(SaveData& out) const;
    void restoreFrom(const SaveData& src);

private:
    GlobalStatBuffs&     buffs_;
    const DataRegistry&  registry_;
    const sf::Font*      font_ = nullptr;

    // factories
    std::unique_ptr<MiniEvolutionFactory>   mini_factory_;
    std::unique_ptr<MajorEvolutionFactory>  major_factory_;
    std::unique_ptr<MythicEvolutionFactory> mythic_factory_;

    // Polimorfism prin pointer la baza (pentru totalAvailableCards si tierName afisaj).
    std::vector<EvolutionFactory*> factories_;

    // Slots de tip "spec, bought (true => card dispare vizual)".
    struct MiniSlot  { MiniStatSpec       spec; bool bought = false; bool valid = false; };
    struct MajorSlot { MajorEvolutionSpec spec; bool bought = false; bool valid = false; };

    std::vector<MiniSlot>  mini_offer_;
    std::vector<MajorSlot> major_offer_;
    std::vector<EvolutionToken> tokens_;

    bool visible_           = false;
    int  active_token_idx_  = -1;
    std::mt19937* refresh_rng_ = nullptr;

    //layout constants (in pixeli)
    static constexpr float SHOP_X = 460.0f;
    static constexpr float SHOP_Y = 60.0f;
    static constexpr float SHOP_W = 1000.0f;
    static constexpr float SHOP_H = 960.0f;

    static constexpr float MINI_CARD_W  = 300.0f;
    static constexpr float MINI_CARD_H  = 140.0f;
    static constexpr float MINI_CARD_Y  = SHOP_Y + 120.0f;
    static constexpr float MAJOR_CARD_W = 470.0f;
    static constexpr float MAJOR_CARD_H = 160.0f;
    static constexpr float MAJOR_CARD_Y = SHOP_Y + 320.0f;
    static constexpr float TOKEN_AREA_Y = SHOP_Y + 540.0f;
    static constexpr float TOKEN_ROW_H  = 50.0f;

    static constexpr float CLOSE_BTN_W = 40.0f;
    static constexpr float CLOSE_BTN_X = SHOP_X + SHOP_W - CLOSE_BTN_W - 10.0f;
    static constexpr float CLOSE_BTN_Y = SHOP_Y + 10.0f;

    static constexpr float REFRESH_BTN_W  = 220.0f;
    static constexpr float REFRESH_BTN_H  = 50.0f;
    static constexpr float REFRESH_BTN_X  = SHOP_X + SHOP_W - REFRESH_BTN_W - 60.0f;
    static constexpr float REFRESH_BTN_Y  = SHOP_Y + 20.0f;
    static constexpr int   REFRESH_COST   = 100;

    // helpers
    sf::FloatRect miniRect(int idx) const;
    sf::FloatRect majorRect(int idx) const;
    sf::FloatRect tokenRect(int idx) const;
    sf::FloatRect closeRect() const;
    sf::FloatRect refreshRect() const;

    sf::Color rarityColor(Evolution::Rarity r) const;

    // Aplica un Mini buff pe GlobalStatBuffs pe spec.target_type (un singur tower type).
    void applyMini(const MiniStatSpec& spec);
    // Aplica un Major STAT pe GlobalStatBuffs pe spec.target_type.
    void applyMajorStat(const MajorEvolutionSpec& spec);

    // Tower-type keys care folosesc stat_field-ul respectiv (data-driven din TowerSpec).
    // Folosit la refresh() pentru random pick target_type.
    std::vector<std::string> towersAffectedByStatKeys(const std::string& stat_field) const;
};
