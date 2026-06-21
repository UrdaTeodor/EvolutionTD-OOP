#include "AdblockerTower.h"

AdblockerTower::AdblockerTower(const TowerSpec& spec, int col, int row)
    : ProjectileTower(spec, "adblocker", col, row) {}

char AdblockerTower::getDisplayChar() const { return 'D'; }

std::unique_ptr<Tower> AdblockerTower::clone() const {
    return std::make_unique<AdblockerTower>(*this);
}
