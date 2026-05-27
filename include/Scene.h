#pragma once

namespace sf {
    class RenderWindow;
    class Event;
}

class Scene {
public:
    virtual ~Scene() = default;

    // Logica per-frame.
    virtual void update(float dt) = 0;

    // Desenare per-frame 
    virtual void render(sf::RenderWindow& window) = 0;

    // (input mouse / tastatura / close).
    virtual void handleEvent(const sf::Event& event) = 0;
};
