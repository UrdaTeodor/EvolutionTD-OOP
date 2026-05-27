#pragma once
#include <vector>
#include <memory>
#include "Scene.h"

namespace sf {
    class RenderWindow;
    class Event;
}

// SceneManager = stack de scene. Doar scena din varf (top) primeste update/render/event.
//
// Tranzitiile push/pop/replace/clear sunt deffered
// tranzitia se buffereaza si se aplica la sfarsitul frameului (applyPending)
class SceneManager {
    std::vector<std::unique_ptr<Scene>> stack_;

    enum class PendingType { NONE, PUSH, POP, REPLACE, REPLACE_ALL, CLEAR };
    PendingType pending_type_  = PendingType::NONE;
    std::unique_ptr<Scene> pending_scene_;

public:
    // Cereri deferred (apelate de scena curenta in mijlocul handleEvent/update).
    void requestPush(std::unique_ptr<Scene> scene);
    void requestPop();
    void requestReplace(std::unique_ptr<Scene> scene);
    // Sterge stack-ul si push noul scene 
    void requestReplaceAll(std::unique_ptr<Scene> scene);
    void requestClear(); 

    // Aplica tranzitia pending
    void applyPending();

    // Forward la top scene (no-op daca stack gol).
    void update(float dt);
    void render(sf::RenderWindow& window);
    void handleEvent(const sf::Event& event);

    bool empty() const { return stack_.empty(); }
};
