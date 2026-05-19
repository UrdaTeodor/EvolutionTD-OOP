#pragma once
#include <string>
#include <ostream>
#include <vector>
#include <memory> 
#include "Enemy.h"

// Tower e clasa de baza abstracta pentru toate tipurile de turnuri.
class Tower {
    std::string name;
    int cost;
    int x, y;        
    float range;     


    bool movable;

public:
    Tower(const std::string& name, int cost, int x, int y, float range);
    virtual ~Tower() = default;

    virtual void update(std::vector<Enemy>& enemies, float deltaTime) = 0;
    virtual char getDisplayChar() const = 0;

    // Necesar pt cc/op= din Game, care detine vector<unique_ptr<Tower>>.
    // Fiecare derivata returneaza make_unique<XxxTower>(*this)
    virtual std::unique_ptr<Tower> clone() const = 0;

    // doar FirewallTower returneaza true
    virtual bool requiresPath() const;


    // Derivatele pot suprascrie daca au logica speciala (ex. Firewall nu beneficiaza).
    virtual void buffRange(float pct);

    // ability MOVABLE: setata din AbilityEvolution; e gratuita pentru toate turnurile.
    void enableMovable();
    bool isMovable() const;

    int getX() const;
    int getY() const;
    int getCost() const;
    float getRange() const;
    const std::string& getName() const;

    friend std::ostream& operator<<(std::ostream& os, const Tower& t);

protected:
    // NVI: derivatele suprascriu doar partea de detalii specifice
    virtual void displayDetails(std::ostream& os) const = 0;
};
