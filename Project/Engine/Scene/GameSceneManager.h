#pragma once
#include <vector>
#include <memory>
#include "Gameobjects/GameObject.h"

class GameSceneManager {
public:
    static GameSceneManager& getInstance() {
        static GameSceneManager instance;
        return instance;
    }
    GameObject* addGameObject() {
        auto obj = std::make_unique<GameObject>();
        GameObject* rawPtr = obj.get();
        m_gameObjects.push_back(std::move(obj));
        return rawPtr;
    }
    void initialize() {
        for (auto& obj : m_gameObjects)
            obj->initialize();
    }

    void update() {
        for (auto& obj : m_gameObjects)
            obj->update();
    }

    void render() {
        for (auto& obj : m_gameObjects)
            obj->render();
    }

private:
    GameSceneManager() = default;
    ~GameSceneManager() = default;
    GameSceneManager(const GameSceneManager&) = delete;
    GameSceneManager& operator=(const GameSceneManager&) = delete;

    std::vector<std::unique_ptr<GameObject>> m_gameObjects;
};
