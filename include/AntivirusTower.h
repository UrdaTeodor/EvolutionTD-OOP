#pragma once
#include "ProjectileTower.h"
#include <memory>

// Turn cu damage mare si atac lent. Toata logica de tras e in ProjectileTower;
// diferentele fata de Adblocker sunt doar de date (TowerSpec din towers.json).
class AntivirusTower : public ProjectileTower {
public:
    AntivirusTower(const TowerSpec& spec, int col, int row);

    char getDisplayChar() const override;
    std::unique_ptr<Tower> clone() const override;
};
