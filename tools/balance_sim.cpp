// Balance sim headless pentru EvolutionTD.



// *************DISCLAIMER: FULL CU AI**************



// Ruleaza N run-uri complete (15 valuri) per strategie-arhetip, fara UI,
// cu aceeasi logica de joc (Game/Wave/Director/buffs) ca jocul real, si
// raporteaza win rate + valul mediu de moarte. Folosit pentru tunarea
// dificultatii spre "roguelike adevarat" (strategia buna castiga <50%).
//
// Limitari (modeleaza un jucator decent, nu perfect):
//  - strategiile fara token_plan cumpara doar Mini + Major STAT
//  - strategia "synergy" cumpara si ability tokens (DoubleShot/MultiTarget/
//    FireTrail) si le aplica direct — modeleaza jucatorul care exploateaza
//    sinergiile multiplicative (upper bound: presupune ca shop-ul ofera mereu
//    token-ul dorit, fara RNG)
//  - nu foloseste mythics, refresh, sell, undo
//
// Build:   cmake --build build --target balance_sim
// Run:     ./build/Debug/balance_sim [runs_per_strategy] (din radacina repo)

#include "Game.h"
#include "DataRegistry.h"
#include "EvolutionFactory.h"
#include "GameException.h"
#include "GlobalStatBuffs.h"
#include "MapSpec.h"
#include "WaveSpec.h"
#include "TowerSpec.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

constexpr int GRID = 20;

// ---------- geometrie drum ----------

std::vector<std::pair<int, int>> expandPath(const std::vector<std::pair<int, int>>& wp) {
    std::vector<std::pair<int, int>> cells;
    for (size_t i = 0; i + 1 < wp.size(); i++) {
        int r1 = wp[i].first,   c1 = wp[i].second;
        int r2 = wp[i + 1].first, c2 = wp[i + 1].second;
        if (r1 == r2) {
            for (int c = std::min(c1, c2); c <= std::max(c1, c2); c++) cells.push_back({r1, c});
        } else {
            for (int r = std::min(r1, r2); r <= std::max(r1, r2); r++) cells.push_back({r, c1});
        }
    }
    return cells;
}

int coverage(int col, int row, float range, const std::vector<std::pair<int, int>>& pathCells) {
    int n = 0;
    for (const auto& [r, c] : pathCells) {
        float dx = static_cast<float>(c - col);
        float dy = static_cast<float>(r - row);
        if (std::sqrt(dx * dx + dy * dy) <= range) ++n;
    }
    return n;
}

// ---------- strategia jucatorului simulat ----------

// Un token de abilitate pe care strategia vrea sa-l cumpere (in ordine).
struct AbilityBuy {
    AbilityType ab;
    const char* tower_key;   // pe ce tip de turn se aplica
    int cost;                // 200 epic / 500 legendary (din evolutions.json)
};

struct Strategy {
    std::string name;
    // cate turnuri din fiecare tip vrea pana la valul w (1-based).
    int (*wantAV)(int w);
    int (*wantAD)(int w);
    int (*wantHP)(int w);
    int (*wantFW)(int w);
    int (*wantMINER)(int w);
    bool buy_minis;
    bool buy_majors;
    // plan de token-uri (gol = nu cumpara abilitati)
    std::vector<AbilityBuy> token_plan;
};

struct RunResult {
    bool win = false;
    int  wave_reached = 1;
    int  hp_left = 0;
};

// type choice mapping din Game::placeTower
enum { T_AV = 1, T_AD = 2, T_HP = 3, T_FW = 4, T_MINER = 5 };

int countType(const Game& game, const std::string& key) {
    int n = 0;
    for (const auto& t : game.getTowers()) if (t->getTypeKey() == key) ++n;
    return n;
}

// Cel mai bun loc liber (coverage maxim / minim) pentru un tip de turn.
bool bestSpot(const Game& game, const DataRegistry& reg, int typeChoice,
              const std::vector<std::pair<int, int>>& pathCells,
              int& outCol, int& outRow) {
    static const char* keys[] = {"antivirus", "adblocker", "honeypot", "firewall", "bytecoinminer"};
    const TowerSpec& spec = reg.getTower(keys[typeChoice - 1]);

    bool onPath[GRID][GRID] = {};
    for (const auto& [r, c] : pathCells) onPath[r][c] = true;

    if (spec.requires_path) {
        // Firewall: celula de drum cam la 60% din traseu (blocheaza tarziu, dupa
        // ce turnurile au tras), fara spawn si fara final.
        for (int off = 0; off < static_cast<int>(pathCells.size()); ++off) {
            int idx = static_cast<int>(pathCells.size() * 6 / 10) + (off % 2 == 0 ? off / 2 : -(off + 1) / 2);
            if (idx < 1 || idx >= static_cast<int>(pathCells.size()) - 1) continue;
            auto [r, c] = pathCells[idx];
            if (!game.towerAt(c, r)) { outCol = c; outRow = r; return true; }
        }
        return false;
    }

    float range   = (spec.range > 0.0f) ? spec.range : 1.5f;
    bool  wantMax = (typeChoice != T_MINER);   // minerul nu iroseste pozitii bune
    int   bestCov = wantMax ? -1 : 1 << 30;
    outCol = -1;
    for (int r = 0; r < GRID; ++r) {
        for (int c = 0; c < GRID; ++c) {
            if (onPath[r][c] || game.towerAt(c, r)) continue;
            int cov = coverage(c, r, range, pathCells);
            if (typeChoice == T_HP && cov == 0) continue;   // honeypot fara drum in raza = inutil
            if (( wantMax && cov > bestCov) ||
                (!wantMax && cov < bestCov)) {
                bestCov = cov;
                outCol = c; outRow = r;
            }
        }
    }
    return outCol >= 0;
}

void tryPlace(Game& game, const DataRegistry& reg, int typeChoice,
              const std::vector<std::pair<int, int>>& pathCells) {
    int col = 0, row = 0;
    if (!bestSpot(game, reg, typeChoice, pathCells, col, row)) return;
    try {
        game.placeTower(typeChoice, col, row);
    } catch (const GameException&) {
        // fara bani / limita atinsa - ignoram, mai incercam la valul urmator
    }
}

// Faza de cumparaturi dintre valuri (inainte de valul `wave`).
void shoppingPhase(Game& game, const DataRegistry& reg, const Strategy& s, int wave,
                   const std::vector<std::pair<int, int>>& pathCells,
                   const MapSpec& map, int& miniRotation, int& minisBought,
                   int& nextToken) {
    struct Want { int type; const char* key; int want; };
    const Want wants[] = {
        { T_AV,    "antivirus",     s.wantAV(wave)    },
        { T_AD,    "adblocker",     s.wantAD(wave)    },
        { T_HP,    "honeypot",      s.wantHP(wave)    },
        { T_FW,    "firewall",      s.wantFW(wave)    },
        { T_MINER, "bytecoinminer", s.wantMINER(wave) },
    };
    for (const auto& w : wants) {
        while (countType(game, w.key) < w.want &&
               game.getMoney() >= reg.getTower(w.key).cost) {
            int before = static_cast<int>(game.getTowers().size());
            tryPlace(game, reg, w.type, pathCells);
            if (static_cast<int>(game.getTowers().size()) == before) break;  // n-a mers
        }
    }

    // Oferta de Major exista doar daca valul tocmai terminat avea offers_major
    // (asa face refresh-ul real din GameScene).
    bool majorOffered = false;
    if (wave >= 2 && wave - 1 <= static_cast<int>(map.wave_ids.size())) {
        majorOffered = reg.getWave(map.wave_ids[wave - 2]).offers_major;
    }

    // Ability tokens (sinergii multiplicative): cumpara urmatorul din plan si
    // aplica-l direct pe primul turn de tipul potrivit. +12 weight ca in shop.
    if (majorOffered && nextToken < static_cast<int>(s.token_plan.size())) {
        const AbilityBuy& tb = s.token_plan[nextToken];
        if (game.getMoney() >= tb.cost + 100) {
            Tower* target = nullptr;
            for (const auto& t : game.getTowers()) {
                if (t->getTypeKey() == tb.tower_key) { target = t.get(); break; }
            }
            if (target) {
                try {
                    target->applyAbility(tb.ab);
                    game.mutableMoney()  -= tb.cost;
                    game.mutableWeight() += 20;
                    ++nextToken;
                } catch (const GameException&) {
                    ++nextToken;   // incompatibil: renunta la acest item din plan
                }
            }
        }
    }

    if (s.buy_majors && majorOffered && game.getMoney() >= 150) {
        const char* target = (miniRotation % 2 == 0) ? "antivirus" : "adblocker";
        game.mutableBuffs().add(target, "damage_pct", 0.20f);
        game.mutableMoney()  -= 100;
        game.mutableWeight() += 12;   // identic cu ShopPanel::handleClick
        ++miniRotation;
    }

    if (s.buy_minis) {
        // Rotatie damage/atkspeed pe av/ad, +4 weight per mini (ca shop-ul real).
        // Pret escaladant ca in ShopPanel: 25 + 3 * minis cumparate pana acum.
        while (true) {
            int price = 25 + 3 * minisBought;
            if (game.getMoney() < price + 50) break;
            const char* target = (miniRotation % 2 == 0) ? "antivirus" : "adblocker";
            const char* stat   = ((miniRotation / 2) % 2 == 0) ? "damage_pct"
                                                               : "attack_speed_pct";
            game.mutableBuffs().add(target, stat, 0.07f);
            game.mutableMoney()  -= price;
            game.mutableWeight() += 4;
            ++miniRotation;
            ++minisBought;
        }
    }
}

RunResult runOne(const DataRegistry& reg, const Strategy& s, unsigned seed,
                 bool hardcore, bool trace = false) {
    Game game(reg);
    game.mutableRng().seed(seed);

    const MapSpec& map = reg.getMap("default");
    auto pathCells = expandPath(map.path);

    int miniRotation = 0;
    int minisBought  = 0;
    int nextToken    = 0;
    RunResult res;

    while (true) {
        shoppingPhase(game, reg, s, game.getWaveNumber(), pathCells, map,
                      miniRotation, minisBought, nextToken);

        int wave_being_played = game.getWaveNumber();
        game.startWave();
        while (game.isWaveActive()) {
            game.tickWave(0.05f);
        }
        if (trace) {
            std::cout << "  w" << wave_being_played
                      << "  HP=" << game.getPlayerHP()
                      << "  money=" << game.getMoney()
                      << "  kills=" << game.getTotalKills()
                      << "  towers=" << game.getTowers().size() << "\n";
        }
        if (game.isGameOver() || game.getCurrentWave().bossEscaped()) {
            res.win          = false;
            res.wave_reached = std::min(game.getWaveNumber(), game.getMaxWaves());
            res.hp_left      = 0;
            return res;
        }
        game.endWave();

        // Oferta de loterie (inainte de w9/w12): ruta hardcore o ACCEPTA mereu
        // (gauntlet 6 valuri HP boostat, +250cr la final), ruta normala refuza.
        // De la calibrarea 2026-06-13, hardcore e modul-tinta pentru balans.
        if (game.lotteryOfferPending()) {
            if (hardcore) game.acceptLottery();
            else          game.declineLottery();
        }
        (void)game.takeGauntletReward();   // banii intra automat; token nu e modelat

        if (game.getWaveNumber() > game.getMaxWaves()) {
            res.win          = true;
            res.wave_reached = game.getMaxWaves();
            res.hp_left      = game.getPlayerHP();
            return res;
        }
    }
}

// ---------- definitia strategiilor ----------

int avBalanced(int w)    { return std::min(6, 1 + w / 3); }
int adBalanced(int w)    { return std::min(6, (w + 1) / 2); }
int hpBalanced(int w)    { return std::min(4, w / 4); }
int fwNone(int)          { return 0; }
int minerBalanced(int w) { return (w >= 3) ? 2 : 0; }

int avEco(int w)    { return (w >= 4) ? std::min(6, w / 2) : 1; }
int adEco(int w)    { return (w >= 5) ? std::min(6, w / 2) : 0; }
int hpEco(int w)    { return std::min(2, w / 6); }
int minerEco(int w) { return std::min(10, 1 + w); }

int avTurtle(int w)    { return std::min(6, 1 + w / 4); }
int adTurtle(int w)    { return std::min(3, w / 4); }
int hpTurtle(int w)    { return std::min(4, w / 3); }
int fwTurtle(int w)    { return std::min(3, 1 + w / 5); }
int minerTurtle(int)   { return 0; }

int zero(int) { return 0; }
int adOnly(int w) { return std::min(6, 1 + w / 2); }

} // namespace

int main(int argc, char** argv) {
    int runs = (argc > 1) ? std::atoi(argv[1]) : 200;
    bool trace = (argc > 2 && std::string(argv[2]) == "trace");

    DataRegistry registry;
    try {
        registry.loadAll("data");
    } catch (const GameException& err) {
        std::cerr << "Eroare la incarcare date: " << err.what() << "\n";
        return 1;
    }

    if (trace) {
        Strategy eco{ "economy-trace", avEco, adEco, hpEco, fwNone, minerEco,
                      true, true, {} };
        std::cout << "=== trace economy seed 1000 (hardcore) ===\n";
        runOne(registry, eco, 1000u, true, true);
        return 0;
    }

    // Mod "rngtest": verifica empiric distributia ofertelor Mini din shop
    // (replicand secventa de draw-uri din ShopPanel::refresh).
    if (argc > 2 && std::string(argv[2]) == "rngtest") {
        MiniEvolutionFactory mini("data/evolutions.json");
        std::map<std::string, int> counts;
        int seeds_without_income_first3 = 0;
        constexpr int SEEDS = 5000;
        std::random_device rd;

        for (int sd = 0; sd < SEEDS; ++sd) {
            std::mt19937 rng(rd());
            bool income_seen = false;
            // 3 oferte (val 1, 2, 3) x 3 carduri; fiecare card = 1 sample +
            // 1 draw de target (ca in ShopPanel::refresh).
            for (int offer = 0; offer < 3; ++offer) {
                for (int card = 0; card < 3; ++card) {
                    const MiniStatSpec& spec = mini.sample(rng);
                    counts[spec.name]++;
                    std::uniform_int_distribution<size_t> d(0, 1);
                    (void)d(rng);   // consuma draw-ul de target ca in joc
                    if (spec.name == "IncomeBuff") income_seen = true;
                }
            }
            if (!income_seen) ++seeds_without_income_first3;
        }

        std::cout << "Distributie mini pe " << SEEDS << " seed-uri (9 carduri/seed):\n";
        for (const auto& [name, n] : counts) {
            std::cout << "  " << name << ": "
                      << (100.0 * n / (SEEDS * 9.0)) << "%  (asteptat ~14.3%)\n";
        }
        std::cout << "Seed-uri FARA income in primele 3 valuri: "
                  << (100.0 * seeds_without_income_first3 / SEEDS)
                  << "%  (asteptat ~24.9%)\n";
        return 0;
    }

    // Planul de sinergii: MultiTarget + DoubleShot stack-uite pe AV/AD
    // (multiplicativ cu mini-urile de damage/atkspeed) + suport (Overclock/
    // Amplify pe honeypot) + AoE (BlastWave pe AV) din v0.4.5.
    const std::vector<AbilityBuy> synergyPlan = {
        { AbilityType::MULTI_TARGET, "antivirus", 300 },
        { AbilityType::DOUBLE_SHOT,  "antivirus", 500 },
        { AbilityType::OVERCLOCK,    "honeypot",  400 },
        { AbilityType::MULTI_TARGET, "adblocker", 300 },
        { AbilityType::SPLASH,       "antivirus", 500 },
        { AbilityType::DOUBLE_SHOT,  "adblocker", 500 },
        { AbilityType::AMPLIFY,      "honeypot",  250 },
        { AbilityType::FIRE_TRAIL,   "antivirus", 500 },
    };

    const Strategy strategies[] = {
        { "balanced (av+ad+honey+miner, minis+majors)",
          avBalanced, adBalanced, hpBalanced, fwNone, minerBalanced, true,  true,  {} },
        { "synergy  (balanced + ability tokens DoubleShot/MultiTarget)",
          avBalanced, adBalanced, hpBalanced, fwNone, minerBalanced, true,  true,  synergyPlan },
        { "economy  (miners first, towers late)",
          avEco,      adEco,      hpEco,      fwNone, minerEco,      true,  true,  {} },
        { "turtle   (firewalls+av, fara economie)",
          avTurtle,   adTurtle,   hpTurtle,   fwTurtle, minerTurtle, true,  false, {} },
        { "naive    (doar adblockere, fara shop)",
          zero,       adOnly,     zero,       fwNone, zero,          false, false, {} },
    };

    // Fiecare strategie ruleaza pe AMBELE rute. Hardcore (loteria acceptata)
    // e modul-tinta pentru calibrare; normal ramane referinta de accesibilitate.
    for (const auto& s : strategies) {
        std::cout << "\n=== " << s.name << " ===\n";
        for (bool hardcore : {false, true}) {
            int wins = 0;
            long long hp_sum = 0;
            std::map<int, int> death_wave_histo;

            for (int i = 0; i < runs; ++i) {
                RunResult r = runOne(registry, s, 1000u + static_cast<unsigned>(i),
                                     hardcore);
                if (r.win) { ++wins; hp_sum += r.hp_left; }
                else       { death_wave_histo[r.wave_reached]++; }
            }

            std::cout << (hardcore ? "  [HARDCORE] " : "  [normal]   ")
                      << "win rate: " << (100.0 * wins / runs)
                      << "%  (" << wins << "/" << runs << ")";
            if (wins > 0) {
                std::cout << "  HP mediu la win: " << (hp_sum / wins);
            }
            std::cout << "\n";
            if (!death_wave_histo.empty()) {
                std::cout << "             morti pe val:";
                for (const auto& [wave, n] : death_wave_histo) {
                    std::cout << "  w" << wave << ":" << n;
                }
                std::cout << "\n";
            }
        }
    }
    return 0;
}
