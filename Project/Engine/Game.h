#pragma once

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <iomanip>

#include "vulkanbase/VulkanBase.h"
#include "Engine/WindowManager.h"
#include "Engine/InputManager.h"
#include "Engine/RendererManager.h"
#include "Engine/PhysxBase.h"
#include "Engine/SceneModelManager.h"
#include "Engine/GameSceneManager.h"

class Game {
public:
    Game();
    ~Game();

    // Initializes the game (window, input, physics, camera, renderer, scene, etc.)
    void init();

    // Main loop that updates and renders the scene
    void run();

private:
    // Helper to setup the scene
    void initScene();

    // Frame timing variables
    bool m_CapFps = true;
    const int m_FPSCap = 60;
    const std::chrono::nanoseconds m_FrameDuration = std::chrono::nanoseconds(1'000'000'000 / m_FPSCap);


    // Core engine objects
    WindowManager& m_WindowManager;
    RendererManager* m_Renderer;
    Camera* m_Camera;

    // For physics and scenes
    PhysxBase& m_Physics;
    SceneModelManager& m_SceneManager;
    GameSceneManager& m_GameScene;

    // Render items list (for example purposes)
    std::vector<RenderItem> m_RenderItems;
};
