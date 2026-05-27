#include "BytecoinMinerTower.h"
#include "GlobalStatBuffs.h"
#include "GameException.h"

BytecoinMinerTower::BytecoinMinerTower(const TowerSpec& spec, int col, int row)
    : Tower(spec, "bytecoinminer", col, row) {}

void BytecoinMinerTower::update(std::vector<Enemy>& /*enemies*/, float /*deltaTime*/,
                                const GlobalStatBuffs& /*buffs*/,
                                const std::vector<std::pair<int, int>>& /*path*/) {
    // Nu ataca.
}

char BytecoinMinerTower::getDisplayChar() const { return 'M'; }

std::unique_ptr<Tower> BytecoinMinerTower::clone() const {
    return std::make_unique<BytecoinMinerTower>(*this);
}

// cppcheck-suppress unusedFunction
int BytecoinMinerTower::collectIncome(const GlobalStatBuffs& buffs) const {
    float multiplier = 1.0f + buffs.for_type(getTypeKey()).income_pct;
    return static_cast<int>(spec().income_per_wave * multiplier);
}

void BytecoinMinerTower::applyAbility(AbilityType a) {
    switch (a) {
        case AbilityType::MOVABLE: enableMovable(); break;
        default:
            Tower::applyAbility(a);
    }
}

void BytecoinMinerTower::displayDetails(std::ostream& os) const {
    os << " income:+" << spec().income_per_wave << "/wave";
}
