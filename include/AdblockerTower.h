#pragma once
#include "ProjectileTower.h"
#include <memory>

// Turn cu damage mic si atac rapid. Toata logica de tras e in ProjectileTower;
// diferentele fata de Antivirus sunt doar de date (TowerSpec din towers.json).
class AdblockerTower : public ProjectileTower {
public:
    AdblockerTower(const TowerSpec& spec, int col, int row);

    char getDisplayChar() const override;
    std::unique_ptr<Tower> clone() const override;
};
