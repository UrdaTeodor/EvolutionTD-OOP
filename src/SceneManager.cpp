#include "SceneManager.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

void SceneManager::requestPush(std::unique_ptr<Scene> scene) {
    pending_type_  = PendingType::PUSH;
    pending_scene_ = std::move(scene);
}

void SceneManager::requestPop() {
    pending_type_ = PendingType::POP;
    pending_scene_.reset();
}

void SceneManager::requestReplace(std::unique_ptr<Scene> scene) {
    pending_type_  = PendingType::REPLACE;
    pending_scene_ = std::move(scene);
}

// cppcheck-suppress unusedFunction
void SceneManager::requestReplaceAll(std::unique_ptr<Scene> scene) {
    pending_type_  = PendingType::REPLACE_ALL;
    pending_scene_ = std::move(scene);
}

void SceneManager::requestClear() {
    pending_type_ = PendingType::CLEAR;
    pending_scene_.reset();
}

void SceneManager::applyPending() {
    switch (pending_type_) {
        case PendingType::NONE:
            break;
        case PendingType::PUSH:
            if (pending_scene_) stack_.push_back(std::move(pending_scene_));
            break;
        case PendingType::POP:
            if (!stack_.empty()) stack_.pop_back();
            break;
        case PendingType::REPLACE:
            if (!stack_.empty()) stack_.pop_back();
            if (pending_scene_) stack_.push_back(std::move(pending_scene_));
            break;
        case PendingType::REPLACE_ALL:
            stack_.clear();
            if (pending_scene_) stack_.push_back(std::move(pending_scene_));
            break;
        case PendingType::CLEAR:
            stack_.clear();
            break;
    }
    pending_type_ = PendingType::NONE;
    pending_scene_.reset();
}

void SceneManager::update(float dt) {
    if (stack_.empty()) return;
    stack_.back()->update(dt);
}

void SceneManager::render(sf::RenderWindow& window) {
    // de deasupra (ex. PauseScene overlay) deseneaza pe deasupra fara sa clear-uieze.
    // Doar top scene primeste update/handleEvent.
    for (auto& s : stack_) {
        s->render(window);
    }
}

void SceneManager::handleEvent(const sf::Event& event) {
    if (stack_.empty()) return;
    stack_.back()->handleEvent(event);
}
