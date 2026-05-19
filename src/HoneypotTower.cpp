#include "HoneypotTower.h"
#include <cmath>

HoneypotTower::HoneypotTower(int col, int row)
    : Tower("Honeypot", 30, col, row, 1.5f), biggerAura(false) {}

std::unique_ptr<Tower> makeHoneypot(int col, int row) {
    return std::make_unique<HoneypotTower>(col, row);
}

void HoneypotTower::update(std::vector<Enemy>& enemies, float /*deltaTime*/) {
    for (auto& enemy : enemies) {
        if (!enemy.isAlive()) continue;
        float dx = enemy.getX() - static_cast<float>(getX());
        float dy = enemy.getY() - static_cast<float>(getY());
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist <= getRange()) {
            enemy.applySlow(0.8f);  // -20% viteza
        }
    }
}

char HoneypotTower::getDisplayChar() const { return 'H'; }

// Virtual ctor: deep-copy polimorfic. cc implicit copiaza toti membrii (Tower + biggerAura).
std::unique_ptr<Tower> HoneypotTower::clone() const {
    return std::make_unique<HoneypotTower>(*this);
}

void HoneypotTower::enableBiggerAura() {
    biggerAura = true;
    buffRange(0.5f);  // efect placeholder: +50% raza
}

void HoneypotTower::displayDetails(std::ostream& os) const {
    os << " slow:20% range:" << getRange();
    if (biggerAura) os << " [BiggerAura]";
}
