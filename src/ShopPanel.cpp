#include "ShopPanel.h"
#include "GlobalStatBuffs.h"
#include "DataRegistry.h"
#include "GameException.h"
#include "SaveData.h"
#include "TowerSpec.h"
#include <algorithm>
#include <utility>

namespace {
    sf::Color withAlpha(sf::Color c, sf::Uint8 a) { c.a = a; return c; }

    void drawText(sf::RenderWindow& window, const sf::Font* font,
                  const std::string& s, unsigned size, float x, float y, sf::Color col) {
        if (!font) return;
        sf::Text t;
        t.setFont(*font);
        t.setString(s);
        t.setCharacterSize(size);
        t.setFillColor(col);
        t.setPosition(x, y);
        window.draw(t);
    }
}

namespace {
    std::string prettyStat(const std::string& s) {
        if (s == "damage_pct")       return "Damage";
        if (s == "range_pct")        return "Range";
        if (s == "attack_speed_pct") return "Attack Speed";
        if (s == "max_hp_pct")       return "Max HP";
        if (s == "regen_pct")        return "Regen";
        if (s == "slow_pct")         return "Slow";
        if (s == "income_pct")       return "Income";
        return s;
    }
}


ShopPanel::ShopPanel(GlobalStatBuffs& buffs, const DataRegistry& registry)
    : buffs_(buffs), registry_(registry) {}

void ShopPanel::loadFromJson(const std::string& path) {
    mini_factory_   = std::make_unique<MiniEvolutionFactory>(path);
    major_factory_  = std::make_unique<MajorEvolutionFactory>(path);
    mythic_factory_ = std::make_unique<MythicEvolutionFactory>(path);

    factories_.clear();
    factories_.push_back(mini_factory_.get());
    factories_.push_back(major_factory_.get());
    factories_.push_back(mythic_factory_.get());
}

void ShopPanel::setFont(const sf::Font& font) {
    font_ = &font;
}

void ShopPanel::refresh(std::mt19937& rng, bool offer_major) {
    refresh_rng_ = &rng;
    mini_offer_.clear();
    major_offer_.clear();

    //  pick random target_type din lista celor afectati de un stat.
    auto pickRandomTarget = [&](const std::string& stat_field) -> std::string {
        auto keys = towersAffectedByStatKeys(stat_field);
        if (keys.empty()) return {};
        std::uniform_int_distribution<size_t> d(0, keys.size() - 1);
        return keys[d(rng)];
    };

    if (mini_factory_ && mini_factory_->size() > 0) {
        for (int i = 0; i < 3; i++) {
            MiniStatSpec spec = mini_factory_->sample(rng);   // copy
            spec.target_type  = pickRandomTarget(spec.stat_field);
            mini_offer_.push_back({ spec, false, true });
        }
    }
    if (offer_major && major_factory_ && major_factory_->size() > 0) {
        for (int i = 0; i < 2; i++) {
            MajorEvolutionSpec spec = major_factory_->sample(rng);   // copy
            if (spec.kind == MajorEvolutionSpec::Kind::STAT) {
                spec.target_type = pickRandomTarget(spec.stat_field);
            }
            major_offer_.push_back({ spec, false, true });
        }
    }
}

int ShopPanel::totalOfferedCards() const {
    int n = 0;
    for (const auto& m : mini_offer_)  if (m.valid && !m.bought) ++n;
    for (const auto& m : major_offer_) if (m.valid && !m.bought) ++n;
    return n;
}

const EvolutionToken* ShopPanel::activeToken() const {
    if (active_token_idx_ < 0 ||
        active_token_idx_ >= static_cast<int>(tokens_.size())) return nullptr;
    return &tokens_[active_token_idx_];
}

void ShopPanel::consumeActiveToken() {
    if (active_token_idx_ < 0 ||
        active_token_idx_ >= static_cast<int>(tokens_.size())) return;
    tokens_.erase(tokens_.begin() + active_token_idx_);
    active_token_idx_ = -1;
}

void ShopPanel::cancelActiveToken() {
    active_token_idx_ = -1;
}

// ---- rects ----

sf::FloatRect ShopPanel::miniRect(int idx) const {
    float gap = (SHOP_W - 3 * MINI_CARD_W) / 4.0f;
    float x = SHOP_X + gap + idx * (MINI_CARD_W + gap);
    return { x, MINI_CARD_Y, MINI_CARD_W, MINI_CARD_H };
}

sf::FloatRect ShopPanel::majorRect(int idx) const {
    float gap = (SHOP_W - 2 * MAJOR_CARD_W) / 3.0f;
    float x = SHOP_X + gap + idx * (MAJOR_CARD_W + gap);
    return { x, MAJOR_CARD_Y, MAJOR_CARD_W, MAJOR_CARD_H };
}

sf::FloatRect ShopPanel::tokenRect(int idx) const {
    return { SHOP_X + 25.0f, TOKEN_AREA_Y + 40.0f + idx * TOKEN_ROW_H,
             SHOP_W - 50.0f, TOKEN_ROW_H - 8.0f };
}

sf::FloatRect ShopPanel::closeRect() const {
    return { CLOSE_BTN_X, CLOSE_BTN_Y, CLOSE_BTN_W, CLOSE_BTN_W };
}

sf::FloatRect ShopPanel::refreshRect() const {
    return { REFRESH_BTN_X, REFRESH_BTN_Y, REFRESH_BTN_W, REFRESH_BTN_H };
}

sf::Color ShopPanel::rarityColor(Evolution::Rarity r) const {
    switch (r) {
        case Evolution::Rarity::MINI:      return sf::Color(140, 180, 200);
        case Evolution::Rarity::RARE:      return sf::Color( 60, 140, 220);
        case Evolution::Rarity::EPIC:      return sf::Color(160,  80, 200);
        case Evolution::Rarity::LEGENDARY: return sf::Color(220, 140,  40);
        case Evolution::Rarity::MYTHIC:    return sf::Color(220,  60,  60);
    }
    return sf::Color::White;
}

// ---- render ----

void ShopPanel::render(sf::RenderWindow& window, int money) const {
    if (!visible_) return;

    // Background semi-transparent overlay.
    sf::RectangleShape bg(sf::Vector2f(SHOP_W, SHOP_H));
    bg.setPosition(SHOP_X, SHOP_Y);
    bg.setFillColor(sf::Color(25, 28, 40, 235));
    bg.setOutlineColor(sf::Color(120, 160, 200));
    bg.setOutlineThickness(3.0f);
    window.draw(bg);

    // Header
    drawText(window, font_, "SHOP", 42, SHOP_X + 30.0f, SHOP_Y + 25.0f,
             sf::Color(220, 230, 255));

    // Close button
    sf::RectangleShape closeBtn(sf::Vector2f(CLOSE_BTN_W, CLOSE_BTN_W));
    closeBtn.setPosition(CLOSE_BTN_X, CLOSE_BTN_Y);
    closeBtn.setFillColor(sf::Color(120, 50, 50));
    closeBtn.setOutlineColor(sf::Color::White);
    closeBtn.setOutlineThickness(2.0f);
    window.draw(closeBtn);
    drawText(window, font_, "X", 24, CLOSE_BTN_X + 12.0f, CLOSE_BTN_Y + 5.0f, sf::Color::White);

    bool canRefresh = (money >= REFRESH_COST);
    sf::RectangleShape refreshBtn(sf::Vector2f(REFRESH_BTN_W, REFRESH_BTN_H));
    refreshBtn.setPosition(REFRESH_BTN_X, REFRESH_BTN_Y);
    refreshBtn.setFillColor(canRefresh ? sf::Color(60, 90, 130) : sf::Color(60, 40, 40));
    refreshBtn.setOutlineColor(sf::Color::White);
    refreshBtn.setOutlineThickness(2.0f);
    window.draw(refreshBtn);
    drawText(window, font_, "REFRESH " + std::to_string(REFRESH_COST) + " cr",
             20, REFRESH_BTN_X + 14.0f, REFRESH_BTN_Y + 14.0f, sf::Color::White);

    // Mini section header
    drawText(window, font_, "MINI EVOLUTIONS",
             20, SHOP_X + 25.0f, MINI_CARD_Y - 30.0f, sf::Color(180, 200, 230));

    for (size_t i = 0; i < mini_offer_.size(); ++i) {
        const auto& slot = mini_offer_[i];
        if (!slot.valid) continue;
        auto r = miniRect(static_cast<int>(i));
        sf::RectangleShape card({r.width, r.height});
        card.setPosition(r.left, r.top);

        if (slot.bought) {
            card.setFillColor(sf::Color(40, 50, 60));
            card.setOutlineColor(sf::Color(80, 80, 80));
        } else {
            bool canAfford = (money >= slot.spec.cost);
            card.setFillColor(canAfford ? sf::Color(50, 70, 100) : sf::Color(50, 40, 40));
            card.setOutlineColor(rarityColor(Evolution::Rarity::MINI));
        }
        card.setOutlineThickness(3.0f);
        window.draw(card);

        if (!slot.bought) {
            std::string target_name = slot.spec.target_type.empty() ? "?"
                : registry_.getTower(slot.spec.target_type).display_name;
            drawText(window, font_,target_name, 24,
                     r.left + 12.0f, r.top + 10.0f, sf::Color(220, 230, 255));
            drawText(window, font_, "+" + std::to_string(int(slot.spec.stat_value * 100.0f)) + "% "
                                    + prettyStat(slot.spec.stat_field),
                     18, r.left + 12.0f, r.top + 45.0f, sf::Color(180, 220, 200));
            drawText(window, font_, std::to_string(slot.spec.cost) + " cr",
                     22, r.left + 12.0f, r.top + r.height - 38.0f, sf::Color(255, 220, 100));
        } else {
            drawText(window, font_, "SOLD", 30, r.left + r.width / 2.0f - 40.0f,
                     r.top + r.height / 2.0f - 22.0f, sf::Color(150, 150, 150));
        }
    }

    // Major section
    drawText(window, font_, "MAJOR EVOLUTIONS",
             20, SHOP_X + 25.0f, MAJOR_CARD_Y - 30.0f, sf::Color(180, 200, 230));

    if (major_offer_.empty()) {
        drawText(window, font_, "(no major offer on this wave)",
                 16, SHOP_X + 25.0f, MAJOR_CARD_Y + 10.0f, sf::Color(120, 140, 160));
    }

    for (size_t i = 0; i < major_offer_.size(); ++i) {
        const auto& slot = major_offer_[i];
        if (!slot.valid) continue;
        auto r = majorRect(static_cast<int>(i));
        sf::RectangleShape card({r.width, r.height});
        card.setPosition(r.left, r.top);

        if (slot.bought) {
            card.setFillColor(sf::Color(40, 50, 60));
            card.setOutlineColor(sf::Color(80, 80, 80));
        } else {
            bool canAfford = (money >= slot.spec.cost);
            card.setFillColor(canAfford ? sf::Color(40, 60, 90) : sf::Color(60, 40, 40));
            card.setOutlineColor(rarityColor(slot.spec.rarity));
        }
        card.setOutlineThickness(3.0f);
        window.draw(card);

        if (!slot.bought) {
            std::string title = slot.spec.name;
            if (slot.spec.kind == MajorEvolutionSpec::Kind::STAT && !slot.spec.target_type.empty()) {
                title += " -> " + registry_.getTower(slot.spec.target_type).display_name;
            }
            drawText(window, font_, title, 22, r.left + 12.0f, r.top + 10.0f,
                     sf::Color(220, 230, 255));
            drawText(window, font_, rarityToString(slot.spec.rarity), 16,
                     r.left + 12.0f, r.top + 42.0f, rarityColor(slot.spec.rarity));

            std::string desc;
            if (slot.spec.kind == MajorEvolutionSpec::Kind::STAT) {
                desc = "+" + std::to_string(int(slot.spec.stat_value * 100.0f)) + "% "
                       + prettyStat(slot.spec.stat_field);
            } else {
                desc = std::string("Ability token: ") + abilityToString(slot.spec.ability);
            }
            drawText(window, font_, desc, 16, r.left + 12.0f, r.top + 70.0f,
                     sf::Color(180, 220, 200));
            drawText(window, font_, std::to_string(slot.spec.cost) + " cr",
                     22, r.left + 12.0f, r.top + r.height - 38.0f, sf::Color(255, 220, 100));
        } else {
            drawText(window, font_, "SOLD", 30, r.left + r.width / 2.0f - 40.0f,
                     r.top + r.height / 2.0f - 22.0f, sf::Color(150, 150, 150));
        }
    }
}

//click handling

ShopPanel::ClickResult ShopPanel::handleClick(float mx, float my, int& money, int& player_weight, std::string& outErr) {
    if (!visible_) return ClickResult::NONE;

    // Click in afara overlay -> ignorat (overlay e modal pentru click-uri in interior).
    if (mx < SHOP_X || mx > SHOP_X + SHOP_W ||
        my < SHOP_Y || my > SHOP_Y + SHOP_H) {
        return ClickResult::NONE;
    }

    if (closeRect().contains(mx, my)) {
        return ClickResult::CLOSE;
    }

    if (refreshRect().contains(mx, my)) {
        if (money < REFRESH_COST) {
            outErr = "Need " + std::to_string(REFRESH_COST) + " cr for refresh";
            return ClickResult::BUY_FAILED;
        }
        if (!refresh_rng_) return ClickResult::NONE;
        money -= REFRESH_COST;
        refresh(*refresh_rng_, true);
        return ClickResult::BUY_OK_INSTANT;
    }

    // Mini cards
    for (size_t i = 0; i < mini_offer_.size(); ++i) {
        auto& slot = mini_offer_[i];
        if (!slot.valid || slot.bought) continue;
        if (!miniRect(static_cast<int>(i)).contains(mx, my)) continue;

        if (money < slot.spec.cost) {
            outErr = "Money insuficient pentru " + slot.spec.name;
            return ClickResult::BUY_FAILED;
        }
        money -= slot.spec.cost;
        applyMini(slot.spec);
        slot.bought   = true;
        player_weight += 10;
        return ClickResult::BUY_OK_INSTANT;
    }

    // Major cards
    for (size_t i = 0; i < major_offer_.size(); ++i) {
        auto& slot = major_offer_[i];
        if (!slot.valid || slot.bought) continue;
        if (!majorRect(static_cast<int>(i)).contains(mx, my)) continue;

        if (money < slot.spec.cost) {
            outErr = "Money insuficient pentru " + slot.spec.name;
            return ClickResult::BUY_FAILED;
        }
        money -= slot.spec.cost;

        if (slot.spec.kind == MajorEvolutionSpec::Kind::STAT) {
            applyMajorStat(slot.spec);
            slot.bought   = true;
            player_weight += 30;
            return ClickResult::BUY_OK_INSTANT;
        } else {
            // Queue FIFO max 3
            if (tokens_.size() >= 3) {
                tokens_.erase(tokens_.begin());
                if (active_token_idx_ == 0)      active_token_idx_ = -1;
                else if (active_token_idx_ > 0)  --active_token_idx_;
            }
            tokens_.push_back({ slot.spec.name, slot.spec.ability, slot.spec.rarity, slot.spec.cost });
            slot.bought   = true;
            player_weight += 30;
            return ClickResult::BUY_OK_TOKEN;
        }
    }

    return ClickResult::NONE;
}

//apply helpers 

void ShopPanel::applyMini(const MiniStatSpec& spec) {
    if (spec.target_type.empty()) return;
    auto& tb = buffs_.mutable_for(spec.target_type);
    if      (spec.stat_field == "damage_pct")       tb.damage_pct       += spec.stat_value;
    else if (spec.stat_field == "range_pct")        tb.range_pct        += spec.stat_value;
    else if (spec.stat_field == "attack_speed_pct") tb.attack_speed_pct += spec.stat_value;
    else if (spec.stat_field == "max_hp_pct")       tb.max_hp_pct       += spec.stat_value;
    else if (spec.stat_field == "regen_pct")        tb.regen_pct        += spec.stat_value;
    else if (spec.stat_field == "slow_pct")         tb.slow_pct         += spec.stat_value;
    else if (spec.stat_field == "income_pct")       tb.income_pct       += spec.stat_value;
}

void ShopPanel::applyMajorStat(const MajorEvolutionSpec& spec) {
    MiniStatSpec adapter{ spec.name, spec.cost, spec.stat_field, spec.stat_value, spec.target_type };
    applyMini(adapter);
}

std::vector<std::string> ShopPanel::towersAffectedByStatKeys(const std::string& stat_field) const {
    std::vector<std::string> result;
    for (const auto& key : registry_.towerKeys()) {
        const TowerSpec& ts = registry_.getTower(key);
        bool affected = false;
        if      (stat_field == "damage_pct")       affected = ts.damage          > 0.0f;
        else if (stat_field == "range_pct")        affected = ts.range           > 0.0f;
        else if (stat_field == "attack_speed_pct") affected = ts.attack_speed    > 0.0f;
        else if (stat_field == "max_hp_pct")       affected = ts.max_hp          > 0.0f;
        else if (stat_field == "regen_pct")        affected = ts.regen_rate      > 0.0f;
        else if (stat_field == "slow_pct")         affected = ts.slow_factor     < 1.0f;
        else if (stat_field == "income_pct")       affected = ts.income_per_wave > 0;
        if (affected) result.push_back(key);
    }
    return result;
}

//Save / Load

namespace {
    const char* kindToStr(MajorEvolutionSpec::Kind k) {
        return k == MajorEvolutionSpec::Kind::STAT ? "STAT" : "ABILITY";
    }

    MajorEvolutionSpec::Kind kindFromStr(const std::string& s) {
        return s == "ABILITY" ? MajorEvolutionSpec::Kind::ABILITY
                              : MajorEvolutionSpec::Kind::STAT;
    }
}

void ShopPanel::serializeTo(SaveData& out) const {
    out.mini_offer.clear();
    for (const auto& slot : mini_offer_) {
        if (!slot.valid) continue;
        out.mini_offer.push_back({
            slot.spec.name, slot.spec.cost,
            slot.spec.stat_field, slot.spec.stat_value,
            slot.spec.target_type,
            slot.bought
        });
    }

    out.major_offer.clear();
    for (const auto& slot : major_offer_) {
        if (!slot.valid) continue;
        SaveData::MajorSlot ms;
        ms.name        = slot.spec.name;
        ms.cost        = slot.spec.cost;
        ms.rarity      = rarityToString(slot.spec.rarity);
        ms.kind        = kindToStr(slot.spec.kind);
        ms.stat_field  = slot.spec.stat_field;
        ms.stat_value  = slot.spec.stat_value;
        ms.ability     = abilityToString(slot.spec.ability);
        ms.target_type = slot.spec.target_type;
        ms.bought      = slot.bought;
        out.major_offer.push_back(std::move(ms));
    }

    out.tokens.clear();
    for (const auto& t : tokens_) {
        out.tokens.push_back({
            t.name,
            abilityToString(t.ability),
            rarityToString(t.rarity),
            t.cost
        });
    }
}

void ShopPanel::restoreFrom(const SaveData& src) {
    mini_offer_.clear();
    for (const auto& ms : src.mini_offer) {
        MiniStatSpec spec{ ms.name, ms.cost, ms.stat_field, ms.stat_value, ms.target_type };
        mini_offer_.push_back({ std::move(spec), ms.bought, true });
    }

    major_offer_.clear();
    for (const auto& ms : src.major_offer) {
        MajorEvolutionSpec spec;
        spec.name        = ms.name;
        spec.cost        = ms.cost;
        spec.rarity      = rarityFromString(ms.rarity);
        spec.kind        = kindFromStr(ms.kind);
        spec.stat_field  = ms.stat_field;
        spec.stat_value  = ms.stat_value;
        spec.target_type = ms.target_type;
        try {
            spec.ability = stringToAbility(ms.ability);
        } catch (...) {
            spec.ability = AbilityType::MULTI_TARGET;
        }
        major_offer_.push_back({ std::move(spec), ms.bought, true });
    }

    tokens_.clear();
    for (const auto& te : src.tokens) {
        try {
            tokens_.push_back({
                te.name,
                stringToAbility(te.ability),
                rarityFromString(te.rarity),
                te.cost
            });
        } catch (...) {
        }
    }
    active_token_idx_ = -1;
}
