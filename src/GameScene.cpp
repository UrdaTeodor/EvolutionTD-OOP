#include "GameScene.h"
#include "SceneManager.h"
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
#include "MythicEvolution.h"
#include "EvolutionContext.h"
#include "WaveSpec.h"
#include "MapSpec.h"
#include "UiLayout.h"
#include <algorithm>
#include <iostream>
#include <string>

namespace {

    bool loadFont(sf::Font& font) {
        const char* paths[] = {
            "assets/font.ttf",                  // bundled (portabil, cautat primul)
            "C:/Windows/Fonts/segoeui.ttf",     // Windows
            "C:/Windows/Fonts/arial.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",         // Debian/Ubuntu
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/dejavu/DejaVuSans.ttf",                  // Fedora
            "/usr/share/fonts/TTF/DejaVuSans.ttf",                     // Arch
            "/System/Library/Fonts/Supplemental/Arial.ttf",           // macOS
            "/Library/Fonts/Arial.ttf",
        };
        for (const char* p : paths) {
            if (font.loadFromFile(p)) return true;
        }
        std::cerr << "GameScene: nu am gasit niciun font.\n";
        return false;
    }

} // namespace

GameScene::GameScene(SceneManager& manager, const DataRegistry& registry)
    : manager_(manager),
      registry_(registry),
      has_font_(loadFont(font_)),
      game_(registry),
      board_(game_.getPath()),
      effects_(sprites_, has_font_ ? &font_ : nullptr, &manager.audio()),
      hud_(has_font_ ? &font_ : nullptr, registry),
      shop_(game_.mutableBuffs(), registry) {
    sprites_.load();

    // Shop: incarca factories + font + offer initial pentru wave 1.
    shop_.loadFromJson("data/evolutions.json");
    if (has_font_) shop_.setFont(font_);
    refreshShopForCurrentWave();
}

GameScene::GameScene(SceneManager& manager, const DataRegistry& registry, const SaveData& save)
    : GameScene(manager, registry) {
    // Restore peste state-ul fresh creat de ctor-ul de mai sus.
    game_.restoreFrom(save);
    shop_.restoreFrom(save);
    board_.rebuild(game_.getPath());
}

void GameScene::cycleGameSpeed() {
    if (game_speed_ == 0) {
        if (wave_running_ || game_.allWavesDone() || game_.isGameOver() ||
            game_.lotteryOfferPending()) {   // popup-ul e modal
            return;
        }
        game_.startWave();
        wave_running_ = true;
        game_speed_   = 1;
        manager_.audio().play(Sfx::WAVE_START, 65.0f);
    }
    else if (game_speed_ == 1) game_speed_ = 2;
    else if (game_speed_ == 2) game_speed_ = 4;
    else                       game_speed_ = 1;   // dupa 4x revine la 1
}

void GameScene::refreshShopForCurrentWave() {
    int wave_num = game_.getWaveNumber();
    const MapSpec& map = registry_.getMap(game_.getMapId());
    bool offers_major = true;
    if (wave_num >= 1 && wave_num <= static_cast<int>(map.wave_ids.size())) {
        offers_major = registry_.getWave(map.wave_ids[wave_num - 1]).offers_major;
    }
    shop_.refresh(game_.mutableRng(), offers_major);
}

void GameScene::saveNow() {
    SaveData d;
    game_.serializeTo(d);
    shop_.serializeTo(d);
    try {
        SaveManager::write(d);
    } catch (const GameException& err) {
        setMessage(std::string("Save fail: ") + err.what());
    }
}

void GameScene::setMessage(const std::string& msg) {
    last_error_ = msg;
    err_clock_.restart();
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

GameScene::TokenApply GameScene::tryApplyActiveTokenOnTower(int col, int row) {
    const EvolutionToken* tok = shop_.activeToken();
    if (!tok) return TokenApply::NO_TARGET;

    Tower* target = game_.towerAt(col, row);
    if (!target) return TokenApply::NO_TARGET;

    if (tok->is_wildcard) {
        // Mythic Token universal: merge doar pe un turn care are deja 2
        // Legendare compatibile (o reteta) aplicate si nu are mythic.
        MythicType m{};
        if (!shop_.mythicTypeFor(*target, m)) return TokenApply::NO_TARGET;
        try {
            target->applyMythic(m);
        } catch (const GameException& err) {
            setMessage(err.what());
            return TokenApply::REJECTED;
        }
        setMessage(std::string("MYTHIC: ") + mythicTypeName(m) + "!");
    } else {
        if (!target->supports(tok->ability)) return TokenApply::NO_TARGET;
        try {
            // KNOCKBACK_EVERY_3 pastreaza dynamic_cast in AbilityEvolution::apply.
            // Restul virtual applyAbility(AbilityType).
            AbilityEvolution applier(tok->name, 0, tok->rarity, tok->ability);
            GlobalStatBuffs& buffs = game_.mutableBuffs();
            EvolutionContext ctx{ &buffs, target->getTypeKey(), target };
            applier.apply(ctx);
        } catch (const GameException& err) {
            // Ex. "nu se stackeaza" — token-ul NU e consumat, motivul ramane pe HUD.
            setMessage(err.what());
            return TokenApply::REJECTED;
        }
    }

    target->recordTokenInvestment(tok->cost);

    shop_.consumeActiveToken();
    token_apply_mode_ = false;
    manager_.audio().play(Sfx::TOKEN, 70.0f);
    return TokenApply::APPLIED;
}

const Tower* GameScene::findTowerAt(int col, int row) const {
    return game_.towerAt(col, row);
}

void GameScene::cycleSelectedTowerTargeting() {
    Tower* t = game_.towerAt(selected_tower_col_, selected_tower_row_);
    if (!t || t->spec().attack_speed <= 0.0f) return;
    t->cycleTargeting();
    setMessage(std::string("Targeting: ") + targetingModeName(t->targeting()));
    saveNow();
}

void GameScene::enterMoveMode() {
    const Tower* t = findTowerAt(selected_tower_col_, selected_tower_row_);
    if (!t || !t->isMovable()) return;
    move_mode_ = true;
    setMessage("Move: click pe celula destinatie (click dreapta = cancel)");
}

void GameScene::sellSelectedTower() {
    const Tower* t = findTowerAt(selected_tower_col_, selected_tower_row_);
    if (!t) return;
    int refund = game_.sellTower(selected_tower_col_, selected_tower_row_);
    selected_tower_col_ = -1;
    selected_tower_row_ = -1;
    manager_.audio().play(Sfx::SELL, 60.0f);
    setMessage("Sold for " + std::to_string(refund) + " cr");
    saveNow();
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
            // Curatare efecte vizuale ramase (proiectile, dare, damage numbers).
            effects_.clear();
            // DUPA clear: monedele de income tasnesc din mineri si zboara
            // spre contorul de bani (platile se vad, nu doar se aduna).
            effects_.spawnIncomeCoins(game_.takeIncomeEvents(),
                                      HudPanel::moneyCounterPos());
            if (!game_.isGameOver()) manager_.audio().play(Sfx::WAVE_END, 60.0f);

            // Recompensa gauntlet-ului de loterie: bani (dati deja in Game) + token.
            if (game_.takeGauntletReward()) {
                shop_.addWildcardToken();
                manager_.audio().play(Sfx::JINGLE_WIN, 85.0f);
                setMessage("LOTTERY COMPLETE: +250 cr si un MYTHIC TOKEN");
            }

            // Save trigger (anti-RNG manipulation): salvam dupa wave end + dupa orice oferta noua.
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
            // NU oprim valul: daca il opream, endWave() nu mai rula, waveNumber
            // ramanea 20, iar urmatorul "start" reconstruia valul 20 cu tot cu
            // boss (bug: boss spawnat de 2 ori).
            if (boss_was_active_ && !bossNow
                && !game_.isEndlessActive()
                && !game_.isGameOver()
                && !game_.getCurrentWave().bossEscaped()) {
                game_.enableEndless();
                manager_.audio().play(Sfx::JINGLE_WIN, 85.0f);
                setMessage("BOSS DOWN - ENDLESS MODE activat");
            }
        }
    }

    // Evenimentele simularii (trageri/impacturi/morti) alimenteaza efectele;
    // sunetele de tragere/lovitura/kill se declanseaza acolo, per eveniment.
    effects_.update(game_, wave_running_, gdt, game_.takeVisualEvents());

    // Efectele mari (firestorm, boss kill) cer screen shake.
    shake_timer_ = std::max(shake_timer_, effects_.takeShakeRequest());

    // Screen shake + vignette rosie + alarma cand pierdem HP (leak).
    if (game_.getPlayerHP() < prev_hp_) {
        shake_timer_ = 0.35f;
        leak_flash_  = 0.4f;
        manager_.audio().play(Sfx::LEAK, 80.0f);
    }
    prev_hp_ = game_.getPlayerHP();
    if (shake_timer_ > 0.0f) shake_timer_ -= dt;
    if (leak_flash_  > 0.0f) leak_flash_  -= dt;

    // Tranzitie deferred la GameOverScene (defeat / victory).
    checkGameOver();
}

std::string GameScene::buildStatus() const {
    if (game_.isGameOver())           return "GAME OVER (ESC -> menu)";
    if (game_.isEndlessActive())      return "ENDLESS MODE - wave " + std::to_string(game_.getWaveNumber());
    if (game_.allWavesDone())         return "VICTORY! (ESC -> menu)";
    if (move_mode_)                   return "Move: click pe celula destinatie\nclick dreapta = cancel";
    if (token_apply_mode_)            return "Apply token: click pe tower verde\nclick in afara = cancel";
    if (game_.isGauntletActive())     return "HARDCORE GAUNTLET\n" +
                                             std::to_string(game_.gauntletWavesLeft()) +
                                             " valuri ramase (HP boostat)";
    if (wave_running_)                return "Wave in curs...";
    if (selected_type_ != 0)          return "Click pe grid sa plasezi";
    return "SPACE = start wave\nU = undo\nESC = menu\nF = fast-forward";
}

void GameScene::render(sf::RenderWindow& window) {
    window.clear(sf::Color(20, 20, 30));

    // Screen shake: translatam view-ul cu un offset care scade spre 0.
    if (shake_timer_ > 0.0f) {
        std::uniform_real_distribution<float> jitter(-1.0f, 1.0f);
        float amp = 7.0f * (shake_timer_ / 0.35f);
        sf::View v = window.getDefaultView();
        v.move(jitter(ui_rng_) * amp, jitter(ui_rng_) * amp);
        window.setView(v);
    } else {
        window.setView(window.getDefaultView());
    }

    hud_.renderBackground(window);
    board_.renderGrid(window);

    sf::Vector2i mouseI = sf::Mouse::getPosition(window);
    float mx = static_cast<float>(mouseI.x);
    float my = static_cast<float>(mouseI.y);
    int hoverCol = ui::pxToCol(mx);
    int hoverRow = ui::pxToRow(my);
    bool inGrid  = ui::inGrid(hoverCol, hoverRow);

    const Tower* hoveredTower = inGrid ? findTowerAt(hoverCol, hoverRow) : nullptr;

    if (hoveredTower) {
        board_.renderTowerRange(window, *hoveredTower, game_.getBuffs());
    } else if (inGrid && selected_type_ != 0) {
        board_.renderPlacementPreview(window, game_, registry_, selected_type_,
                                      hoverCol, hoverRow);
    }

    // Token apply mode: halo verde pe tower-ele compatibile (sub sprite-uri).
    if (token_apply_mode_ && shop_.hasActiveToken()) {
        const EvolutionToken* tok = shop_.activeToken();
        board_.renderTokenHalos(window, game_, [&](const Tower& t) {
            if (tok->is_wildcard) {
                MythicType m{};
                return shop_.mythicTypeFor(t, m);
            }
            return t.supports(tok->ability);
        });
    }

    board_.renderTowers(window, game_, sprites_, effects_);
    effects_.renderTrails(window);
    board_.renderEnemies(window, game_, sprites_);
    effects_.renderForeground(window, game_);

    // Vignette rosie pe margini cand un inamic a intrat in baza: intr-un
    // roguelike, HP-ul pierdut trebuie sa se simta, nu doar sa scada un numar.
    if (leak_flash_ > 0.0f) {
        float t = leak_flash_ / 0.4f;
        sf::Uint8 a = static_cast<sf::Uint8>(110.0f * t);
        const float W = static_cast<float>(ui::WIN_W);
        const float H = static_cast<float>(ui::WIN_H);
        const float B = 70.0f;   // grosimea benzilor
        sf::Color col(220, 30, 30, a);
        sf::RectangleShape band;
        band.setFillColor(col);
        band.setSize({W, B});                       // sus
        band.setPosition(0.0f, 0.0f);
        window.draw(band);
        band.setPosition(0.0f, H - B);              // jos
        window.draw(band);
        band.setSize({B, H});                       // stanga
        band.setPosition(0.0f, 0.0f);
        window.draw(band);
        band.setPosition(W - B, 0.0f);              // dreapta
        window.draw(band);
    }

    HudPanel::Frame frame;
    frame.selected_type = selected_type_;
    frame.game_speed    = game_speed_;
    frame.status        = buildStatus();
    if (!last_error_.empty() && err_clock_.getElapsedTime().asSeconds() < 3.0f) {
        frame.error = last_error_;
    }
    frame.mouse_x = mx;
    frame.mouse_y = my;
    hud_.renderFront(window, game_, shop_, frame);

    // Tower info panel (apare cand player a dat click pe un tower).
    hud_.renderTowerInfo(window, findTowerAt(selected_tower_col_, selected_tower_row_),
                         game_.getBuffs(), mx, my);

    // Token hover popup, sub shop overlay cand shop-ul e deschis.
    hud_.renderTokenPopup(window, shop_, mx, my);

    // Preview Mythic: wildcard activ + hover pe turn compatibil => ce va face.
    if (token_apply_mode_ && shop_.hasActiveToken() &&
        shop_.activeToken()->is_wildcard && hoveredTower) {
        MythicType m{};
        if (shop_.mythicTypeFor(*hoveredTower, m)) {
            hud_.renderMythicPreview(window, mx, my, m);
        }
    }

    // Shop overlay desenat ULTIMUL (peste tot restul), doar daca visible.
    shop_.render(window, game_.getMoney());

    // Gauntlet activ: scena se intuneca (ruta hardcore).
    if (game_.isGauntletActive()) {
        sf::RectangleShape dark({static_cast<float>(ui::WIN_W),
                                 static_cast<float>(ui::WIN_H)});
        dark.setFillColor(sf::Color(15, 0, 25, 80));
        window.draw(dark);
    }

    // Popup-ul de loterie, modal peste absolut tot.
    if (game_.lotteryOfferPending()) {
        sf::RectangleShape dim({static_cast<float>(ui::WIN_W),
                                static_cast<float>(ui::WIN_H)});
        dim.setFillColor(sf::Color(0, 0, 0, 120));
        window.draw(dim);
        hud_.renderLotteryPopup(window);
    }
}

void GameScene::onLeftClick(float mx, float my) {
    // 0. Popup-ul de loterie e modal: nimic altceva nu primeste click-uri.
    if (game_.lotteryOfferPending()) {
        switch (hud_.lotteryHit(mx, my)) {
            case HudPanel::LotteryHit::REDEEM:
                game_.acceptLottery();
                manager_.audio().play(Sfx::TOKEN, 80.0f);
                setMessage("HARDCORE: urmatoarele 6 valuri au HP boostat. Supravietuieste!");
                saveNow();
                break;
            case HudPanel::LotteryHit::CLOSE:
                game_.declineLottery();
                setMessage("Ai inchis reclama. (Poate mai apare...)");
                saveNow();
                break;
            case HudPanel::LotteryHit::NONE:
                break;
        }
        return;
    }

    // 1. Shop overlay are prioritate (modal pentru click-uri in interiorul lui).
    if (shop_.visible()) {
        std::string err;
        ShopPanel::ClickResult res = shop_.handleClick(mx, my,
            game_.mutableMoney(), game_.mutableWeight(), err);
        switch (res) {
            case ShopPanel::ClickResult::CLOSE:
                shop_.setVisible(false);
                token_apply_mode_ = false;
                return;
            case ShopPanel::ClickResult::BUY_FAILED:
                manager_.audio().play(Sfx::ERROR, 55.0f);
                setMessage(err);
                return;
            case ShopPanel::ClickResult::BUY_OK_INSTANT:
                // Mini sau Major STAT: aplicat instant pe buffs in handleClick.
                manager_.audio().play(Sfx::BUY, 65.0f);
                saveNow();
                return;
            case ShopPanel::ClickResult::BUY_OK_TOKEN:
                manager_.audio().play(Sfx::BUY, 65.0f);
                saveNow();
                return;
            case ShopPanel::ClickResult::TOKEN_PICKED:
                token_apply_mode_ = true;
                setMessage("Apply token: click pe tower compatibil (halo verde) sau in afara grid pentru cancel");
                return;
            case ShopPanel::ClickResult::NONE:
                // Click in afara overlay-ului (sau pe slot deja vandut) - cade jos.
                break;
        }
    }

    // 2. Elemente de HUD (token slots, START/FF, shop toggle, butoane tower).
    HudPanel::Hit hit = hud_.hitTest(mx, my, shop_.tokenCount());
    switch (hit.kind) {
        case HudPanel::Hit::Kind::TOKEN_SLOT:
            shop_.setActiveTokenIndex(hit.index);
            token_apply_mode_ = true;
            setMessage("Apply token: click pe tower compatibil (halo verde) sau in afara grid pentru cancel");
            return;
        case HudPanel::Hit::Kind::FF:
            cycleGameSpeed();
            return;
        case HudPanel::Hit::Kind::SHOP_TOGGLE:
            shop_.toggle();
            if (!shop_.visible() && token_apply_mode_) {
                shop_.cancelActiveToken();
                token_apply_mode_ = false;
            }
            return;
        case HudPanel::Hit::Kind::TOWER_BUTTON:
            selected_type_ = hit.index;
            return;
        case HudPanel::Hit::Kind::NONE:
            break;
    }

    // 3. Butoanele MOVE / TARGET / SELL din info panel (daca e un tower selectat).
    if (const Tower* sel = findTowerAt(selected_tower_col_, selected_tower_row_)) {
        if (hud_.moveButtonContains(mx, my, *sel)) {
            enterMoveMode();
            return;
        }
        if (hud_.targetButtonContains(mx, my, *sel)) {
            cycleSelectedTowerTargeting();
            return;
        }
        if (hud_.sellButtonContains(mx, my, *sel)) {
            sellSelectedTower();
            return;
        }
    }

    // 4. Click pe grid.
    int col = ui::pxToCol(mx);
    int row = ui::pxToRow(my);
    bool inGrid = ui::inGrid(col, row);

    if (move_mode_) {
        if (inGrid) {
            try {
                game_.moveTower(selected_tower_col_, selected_tower_row_, col, row);
                selected_tower_col_ = col;
                selected_tower_row_ = row;
                setMessage("Turn mutat");
                saveNow();
            } catch (const GameException& err) {
                setMessage(err.what());
            }
        }
        move_mode_ = false;
        return;
    }

    if (token_apply_mode_) {
        TokenApply res = inGrid ? tryApplyActiveTokenOnTower(col, row)
                                : TokenApply::NO_TARGET;
        if (res != TokenApply::APPLIED) {
            shop_.cancelActiveToken();
            token_apply_mode_ = false;
            if (res == TokenApply::REJECTED) {
                // Motivul refuzului e deja pe HUD (ex. "nu se stackeaza").
                manager_.audio().play(Sfx::ERROR, 55.0f);
            } else {
                setMessage("Cancelled apply mode");
            }
        }
        return;
    }

    if (inGrid) {
        if (findTowerAt(col, row)) {
            selected_tower_col_ = col;
            selected_tower_row_ = row;
            return;
        }
        if (selected_type_ != 0) {
            try {
                game_.placeTower(selected_type_, col, row);
                manager_.audio().play(Sfx::PLACE, 60.0f);
                saveNow();
            } catch (const GameException& err) {
                manager_.audio().play(Sfx::ERROR, 50.0f);
                setMessage(err.what());
            }
            return;
        }
    }

    selected_tower_col_ = -1;
    selected_tower_row_ = -1;
}

void GameScene::handleEvent(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape) {
            // ESC = pauza (push overlay). Game-ul ingheata (SceneManager update doar top).
            manager_.requestPush(std::make_unique<PauseScene>(manager_, *this, registry_));
            return;
        }
        if (event.key.code == sf::Keyboard::Space || event.key.code == sf::Keyboard::F) {
            // SPACE / F: pornire wave la 0->1, fastforward 1->2->4->1.
            cycleGameSpeed();
        }
        if (event.key.code == sf::Keyboard::U) {
            if (game_.restoreSnapshot()) {
                setMessage(">>> Undo: revenit la wavestart " + std::to_string(game_.getWaveNumber()));
                wave_running_    = false;
                boss_was_active_ = false;
                game_speed_      = 0;
                effects_.clear();
            }
        }
        if (event.key.code == sf::Keyboard::S && selected_tower_col_ >= 0) {
            sellSelectedTower();
        }
        if (event.key.code == sf::Keyboard::T && selected_tower_col_ >= 0) {
            cycleSelectedTowerTargeting();
        }
        if (event.key.code == sf::Keyboard::M && selected_tower_col_ >= 0) {
            enterMoveMode();
        }
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Right) {
        selected_type_      = 0;
        selected_tower_col_ = -1;
        selected_tower_row_ = -1;
        move_mode_          = false;
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left) {
        onLeftClick(static_cast<float>(event.mouseButton.x),
                    static_cast<float>(event.mouseButton.y));
    }
}
