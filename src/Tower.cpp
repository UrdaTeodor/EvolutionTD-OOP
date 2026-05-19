#include "Tower.h"

Tower::Tower(const std::string& name, int cost, int x, int y, float range)
    : name(name), cost(cost), x(x), y(y), range(range), movable(false) {}

//doar Firewall o suprascrie sa returneze true.
bool Tower::requiresPath() const { return false; }

void Tower::buffRange(float pct) {
    range *= (1.0f + pct);
}

void Tower::enableMovable() { movable = true; }
// cppcheck-suppress unusedFunction // T3
bool Tower::isMovable() const { return movable; }

int                Tower::getX()     const { return x; }
int                Tower::getY()     const { return y; }
int                Tower::getCost()  const { return cost; }
float              Tower::getRange() const { return range; }
const std::string& Tower::getName()  const { return name; }

std::ostream& operator<<(std::ostream& os, const Tower& t) {
    os << "[" << t.name << " @(" << t.x << "," << t.y << ")";
    t.displayDetails(os);  // -> derivata-specifica
    if (t.movable) os << " [Movable]";
    os << " cost:" << t.cost << "]";
    return os;
}
