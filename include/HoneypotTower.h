#pragma once
#include "Tower.h"
#include <memory>

class HoneypotTower : public Tower {
    // evo  flag
    bool biggerAura;

public:
    HoneypotTower(int col, int row);

    void update(std::vector<Enemy>& enemies, float deltaTime) override;
    char getDisplayChar() const override;
    std::unique_ptr<Tower> clone() const override;

    void enableBiggerAura();

protected:
    void displayDetails(std::ostream& os) const override;
};

std::unique_ptr<Tower> makeHoneypot(int col, int row);
