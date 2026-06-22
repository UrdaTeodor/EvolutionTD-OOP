#include "Tower.h"
#include "GlobalStatBuffs.h"
#include "GameException.h"
#include <algorithm>
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

void Tower::enableMovable() { movable_ = true; }

// cppcheck-suppress unusedFunction
void Tower::recordTokenInvestment(int cost) { token_investment_ += cost; }
// cppcheck-suppress unusedFunction
int  Tower::getTokenInvestment() const      { return token_investment_; }

int Tower::getX()       const { return x_; }
int Tower::getY()       const { return y_; }
int Tower::getCost()    const { return spec_->cost; }
// cppcheck-suppress unusedFunction
float Tower::getRange() const { return spec_->range; }


const std::string& Tower::getName()     const { return spec_->display_name; }
const std::string& Tower::getTypeKey()  const { return type_key_; }
const TowerSpec&   Tower::spec()        const { return *spec_; }

float Tower::effectiveRange(const GlobalStatBuffs& buffs) const {
    return spec_->range * (1.0f + buffs.pct(type_key_, "range_pct"));
}

// cppcheck-suppress unusedFunction
bool Tower::supports(AbilityType ab) const {
    const auto& list = spec_->supports_abilities;
    const std::string ab_str = abilityToString(ab);
    return std::find(list.begin(), list.end(), ab_str) != list.end();
}

std::vector<AbilityType> Tower::getAppliedAbilities() const {
    std::vector<AbilityType> result;
    if (movable_) result.push_back(AbilityType::MOVABLE);
    return result;
}

std::ostream& operator<<(std::ostream& os, const Tower& t) {
    os << "[" << t.getName() << " @(" << t.x_ << "," << t.y_ << ")";
    t.displayDetails(os);
    if (t.movable_) os << " [Movable]";
    os << " cost:" << t.getCost() << "]";
    return os;
}
