#include "BytecoinMinerTower.h"

BytecoinMinerTower::BytecoinMinerTower(int col, int row)
    : Tower("BytecoinMiner", 50, col, row, 0.0f) {}

std::unique_ptr<Tower> makeBytecoinMiner(int col, int row) {
    return std::make_unique<BytecoinMinerTower>(col, row);
}

// nu ataca, deci update e no-op
void BytecoinMinerTower::update(std::vector<Enemy>& /*enemies*/, float /*deltaTime*/) {}

char BytecoinMinerTower::getDisplayChar() const { return 'M'; }

std::unique_ptr<Tower> BytecoinMinerTower::clone() const {
    return std::make_unique<BytecoinMinerTower>(*this);
}

// venit pasiv la finalul fiecarui val. Cost 50 => ROI in 2 valuri.
int BytecoinMinerTower::collectIncome() { return 25; }

void BytecoinMinerTower::displayDetails(std::ostream& os) const {
    os << " income:+25/wave";
}
