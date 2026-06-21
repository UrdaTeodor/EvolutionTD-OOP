#pragma once
#include <SFML/Graphics/Color.hpp>
#include <string>
#include "Evolution.h"

// Culorile "de identitate" ale jocului, partajate intre BoardRenderer, HudPanel
// si ShopPanel. Inainte erau duplicate in GameScene.cpp si ShopPanel.cpp si
// puteau diverge la o modificare.
namespace palette {

    inline sf::Color towerColor(char c) {
        switch (c) {
            case 'A': return sf::Color( 80, 140, 230);
            case 'D': return sf::Color(230, 210,  60);
            case 'H': return sf::Color(230, 150,  60);
            case 'F': return sf::Color(180,  80, 200);
            case 'M': return sf::Color(220, 180,  60);
            default:  return sf::Color::White;
        }
    }

    // Culoarea "exploziei de moarte" per tip de inamic (particulele din
    // EffectsLayer). Aproximeaza tenta dominanta a sprite-ului fiecaruia.
    inline sf::Color enemyColor(const std::string& name) {
        if (name == "Adware")   return sf::Color(255, 200,  80);   // popup galben
        if (name == "Trojan")   return sf::Color(190,  90, 220);   // mov
        if (name == "Worm")     return sf::Color(110, 220, 110);   // verde
        if (name == "ILOVEYOU") return sf::Color(255,  90, 130);   // roz-rosu
        return sf::Color(230, 90, 70);
    }

    inline sf::Color rarityColor(Evolution::Rarity r) {
        switch (r) {
            case Evolution::Rarity::MINI:      return sf::Color(140, 180, 200);
            case Evolution::Rarity::RARE:      return sf::Color( 60, 140, 220);
            case Evolution::Rarity::EPIC:      return sf::Color(160,  80, 200);
            case Evolution::Rarity::LEGENDARY: return sf::Color(220, 140,  40);
            case Evolution::Rarity::MYTHIC:    return sf::Color(220,  60,  60);
        }
        return sf::Color::White;
    }

} // namespace palette
