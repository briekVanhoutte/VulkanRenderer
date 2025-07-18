#pragma once

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <iomanip>

#include <Engine/Core/WindowManager.h>
#include <Engine/Input/InputManager.h>
#include <Engine/Graphics/RendererManager.h>
#include <Engine/Physics/PhysxBase.h>
#include <Engine/Scene/SceneModelManager.h>
#include <Engine/Scene/GameSceneManager.h>

class Game {
public:
    Game();
    ~Game();

    void init();
    void run();

private:
    void initScene();

    bool m_CapFps = false;
    const int m_FPSCap = 60;
    const std::chrono::nanoseconds m_FrameDuration = std::chrono::nanoseconds(1'000'000'000 / m_FPSCap);

    WindowManager& m_WindowManager;
    RendererManager* m_Renderer;
    Camera* m_Camera;

    PhysxBase& m_Physics;
    SceneModelManager& m_SceneManager;
    GameSceneManager& m_GameScene;

    std::vector<RenderItem> m_RenderItems;
};
