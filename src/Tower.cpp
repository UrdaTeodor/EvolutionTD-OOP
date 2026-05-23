#include "Tower.h"
#include "GlobalStatBuffs.h"
#include "GameException.h"
#include <utility>

Tower::Tower(const TowerSpec& spec, std::string type_key, int x, int y)
    : spec_(&spec), type_key_(std::move(type_key)), x_(x), y_(y), movable_(false) {}

void Tower::applyAbility(AbilityType /*a*/) {
    throw IncompatibleEvolutionException(
        "Turnul '" + spec_->display_name + "' nu suporta aceasta abilitate.");
}

int Tower::collectIncome(const GlobalStatBuffs& /*buffs*/) const {
    return 0;   // turnurile nonMiner nu dau venit
}

bool Tower::requiresPath() const { return spec_->requires_path; }

void Tower::enableMovable() { movable_ = true; }
// cppcheck-suppress unusedFunction // T3  
bool Tower::isMovable() const { return movable_; }

int                Tower::getX()        const { return x_; }
int                Tower::getY()        const { return y_; }
int                Tower::getCost()     const { return spec_->cost; }
float              Tower::getRange()    const { return spec_->range; }
const std::string& Tower::getName()     const { return spec_->display_name; }
const std::string& Tower::getTypeKey()  const { return type_key_; }
const TowerSpec&   Tower::spec()        const { return *spec_; }

float Tower::effectiveRange(const GlobalStatBuffs& buffs) const {
    return spec_->range * (1.0f + buffs.for_type(type_key_).range_pct);
}

std::ostream& operator<<(std::ostream& os, const Tower& t) {
    os << "[" << t.getName() << " @(" << t.x_ << "," << t.y_ << ")";
    t.displayDetails(os);
    if (t.movable_) os << " [Movable]";
    os << " cost:" << t.getCost() << "]";
    return os;
}
