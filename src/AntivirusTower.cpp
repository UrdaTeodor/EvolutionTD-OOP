#include "AntivirusTower.h"

AntivirusTower::AntivirusTower(const TowerSpec& spec, int col, int row)
    : ProjectileTower(spec, "antivirus", col, row) {}

char AntivirusTower::getDisplayChar() const { return 'A'; }

std::unique_ptr<Tower> AntivirusTower::clone() const {
    return std::make_unique<AntivirusTower>(*this);
}
