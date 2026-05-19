#pragma once
#include "Tower.h"
#include <memory>

// BytecoinMinerTower: tower off-path care nu ataca, doar genereaza venit pasiv.
// La finalul fiecarui val, returneaza 25 credits prin collectIncome().
// Cost 50 => ROI in 2 valuri.
class BytecoinMinerTower : public Tower {
public:
    BytecoinMinerTower(int col, int row);

    void update(std::vector<Enemy>& enemies, float deltaTime) override;
    char getDisplayChar() const override;
    std::unique_ptr<Tower> clone() const override;
    int collectIncome() override;

protected:
    void displayDetails(std::ostream& os) const override;
};

std::unique_ptr<Tower> makeBytecoinMiner(int col, int row);
