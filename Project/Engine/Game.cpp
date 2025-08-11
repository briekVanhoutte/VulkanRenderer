#include "Game.h"
#include <Engine/Graphics/vulkanVars.h>
#include "Platform/Windows/PlatformWindow_Windows.h"
#include <Engine/Graphics/MaterialManager.h>

Game::Game()
    : m_WindowManager(WindowManager::GetInstance()),
    m_Physics(PhysxBase::GetInstance()),
    m_SceneManager(SceneModelManager::getInstance()),
    m_GameScene(GameSceneManager::getInstance()),
    m_Renderer(nullptr),
    m_Camera(nullptr)
{
}

Game::~Game() {
    delete m_Renderer;
    delete m_Camera;
}

void Game::init() {
    m_WindowManager.initWindow();

#ifdef WIN32
    auto* winPtr = static_cast<PlatformWindow_Windows*>(m_WindowManager.getPlatformWindow());
    InputManager::GetInstance().Initialize(winPtr->getGLFWwindow());
#else
#error not implemented for this os!
#endif
     
    m_Physics.initPhysics(false);

    float fov = 90.f;
    float aspectRatio = float(WIDTH) / float(HEIGHT);
    glm::vec3 cameraStartLocation{ -4.f, -3.f, -25.f };

    m_Camera = new Camera{};
    m_Camera->Initialize(fov, cameraStartLocation, aspectRatio);
    
    m_Renderer = new RendererManager();
    m_Renderer->Initialize();

    initScene();

    m_RenderItems.push_back(RenderItem{ m_SceneManager.getMeshScene(), 0 });
    m_RenderItems.push_back(RenderItem{ m_SceneManager.getParticleScene(), 1 });
}

void Game::initScene() {
    auto& vulkan_vars = vulkanVars::GetInstance();

    // ---- Camera basis (replace forward/up with your actual camera orientation if you have it) ----
    const glm::vec3 camPos = { -4, -3, -25 };          // e.g. {-4, -3, -25}
    glm::vec3 camFwd = glm::normalize(glm::vec3(0.f, 0.f, -1.f)); // TODO: replace with your camera forward
    glm::vec3 camUp = glm::vec3(0.f, 1.f, 0.f);                  // TODO: replace with your camera up
    const float toDeg = 180.0f / 3.1415926535f;
    // Yaw that turns a +Z-facing object toward camFwd (GLM right-handed, -Z forward)
    const float yawDeg = std::atan2(camFwd.x, -camFwd.z) * toDeg;

    // --- 1) One particle group at world center (unchanged) ---------------------
    glm::vec3 posParticles{ 0.f, 0.f, 0.f };
    glm::vec3 scaleParticles{ 1.f, 1.f, 1.f };
    glm::vec3 rotParticles{ 0.f, 0.f, 0.f };

    if (auto* particleBuffer = m_Physics.getParticleBuffer()) {
        auto* positions = particleBuffer->getPositionInvMasses();
        int nbActive = particleBuffer->getNbActiveParticles();
        if (positions && nbActive > 0) {
            m_SceneManager.addParticleGroup(
                positions, nbActive, m_Physics.getParticles(),
                posParticles, scaleParticles, rotParticles);
        }
        else {
            std::cout << "Particle buffer is empty or invalid!\n";
        }
    }
    else {
        std::cout << "No particle buffer available!\n";
    }

    // --- 2) Materials ----------------------------------------------------------
    std::shared_ptr<Material> stoneMat = std::make_shared<Material>(
        "Resources/Textures/Rocks/rocks_albedo.jpg",
        "Resources/Textures/Rocks/rocks_normal.jpg",
        "",
        "Resources/Textures/Rocks/rocks_roughness.jpg",
        "Resources/Textures/Rocks/rocks_displacement.jpg");
    std::shared_ptr<Material> bronzeMat = std::make_shared<Material>("Resources/Textures/errorTexture.jpg");
    std::shared_ptr<Material> testMat = std::make_shared<Material>("Resources/Textures/testTexture.jpg");
    std::shared_ptr<Material> errorMat = std::make_shared<Material>();

    // --- 3) Big ground plane centered under the camera -------------------------
    {
        GameObject* ground = m_GameScene.addGameObject();
        // Center the huge ground under the current camera XZ, a bit below the camera Y
        ground->getTransform()->position = glm::vec3(camPos.x, camPos.y - 1.0f, camPos.z);
        ground->getTransform()->scale = glm::vec3(1.f);
        ground->getTransform()->rotation = glm::vec3(0.f, 0.f, 0.f); // horizontal, normal up
        ground->addComponent<PrimitiveMeshComponent>(
            ground, PrimitiveType::Plane, 200.f, 200.f, 1.f, stoneMat);
    }

    // --- 4) A ring of cubes around the camera (so you see them immediately) ----
    {
        const int   count = 8;
        const float radius = 12.f;
        const float baseY = camPos.y + 0.75f;  // slightly above ground relative to camera

        // Center the ring at the camera XZ (keeps things visible from your POV)
        glm::vec3 ringCenter = glm::vec3(camPos.x, baseY, camPos.z);

        for (int i = 0; i < count; ++i) {
            float t = (i + 0.5f) / float(count);
            float ang = t * (2.0f * 3.1415926535f);

            glm::vec3 pos = ringCenter + glm::vec3(radius * glm::cos(ang), 0.f, radius * glm::sin(ang));

            GameObject* cube = m_GameScene.addGameObject();
            cube->getTransform()->position = pos;
            cube->getTransform()->scale = glm::vec3(1.f);
            // Face roughly toward the ring center:
            cube->getTransform()->rotation = glm::vec3(0.f, -ang * toDeg, 0.f);

            float w = 1.5f;
            float h = 2.0f + 0.75f * (0.5f + 0.5f * glm::cos(ang * 3.0f));
            float d = 1.5f;
            std::shared_ptr<Material> mat = (i % 2 == 0) ? bronzeMat : testMat;

            cube->addComponent<PrimitiveMeshComponent>(cube, PrimitiveType::Cube, w, h, d, mat);
        }
    }

    // --- 5) Backdrop wall placed in front of the camera and facing it ----------
    {
        GameObject* wall = m_GameScene.addGameObject();
        const float wallDist = 30.f;                 // distance in front of camera
        const float wallHeight = 10.f;                 // raise wall center a bit
        wall->getTransform()->position = camPos + camFwd * wallDist + camUp * wallHeight;

        wall->getTransform()->scale = glm::vec3(1.f);
        // Rotate plane from "up-facing" to vertical (-90° X), then yaw to face camera forward
        wall->getTransform()->rotation = glm::vec3(-90.f, yawDeg, 0.f);

        wall->addComponent<PrimitiveMeshComponent>(
            wall, PrimitiveType::Plane, 100.f, 40.f, 1.f, stoneMat);
    }

    // --- 6) Initialize scene + GPU resources -----------------------------------
    m_GameScene.initialize();
    m_SceneManager.initScenes(
        vulkan_vars.physicalDevice,
        vulkan_vars.device,
        vulkan_vars.commandPoolModelPipeline.m_CommandPool,
        vulkan_vars.graphicsQueue);
}

void Game::run() {
    using clock = std::chrono::steady_clock;
    auto lastTime = clock::now();
    float totalTime = 0.f;
    int frameCount = 0;
    auto& vulkan_vars = vulkanVars::GetInstance();
    auto startEngine = clock::now();

    PlatformWindow* window = m_WindowManager.getPlatformWindow();

    while (!window->shouldClose()) {
        auto frameStart = clock::now();
        float deltaTime = std::chrono::duration<float>(frameStart - lastTime).count();
        lastTime = frameStart;

        vulkan_vars.currentFrame = frameCount;

        window->pollEvents();

        //m_Physics.stepPhysics(false, deltaTime);
        m_Renderer->RenderFrame(m_RenderItems, *m_Camera);

        InputManager::GetInstance().HandleCameraInputs(m_Camera, deltaTime);
        m_Camera->update();

        frameCount++;
        totalTime += deltaTime;
        if (totalTime >= 1.0f) {
            std::cout << "FPS: " << std::fixed << std::setprecision(2) << frameCount << std::endl;
            frameCount = 0;
            totalTime = 0.f;
        }

        auto frameEnd = clock::now();

        if (m_CapFps)
        {
            auto frameElapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(frameEnd - frameStart);

            if (frameElapsed < m_FrameDuration) {
                auto sleepTime = m_FrameDuration - frameElapsed;

                if (sleepTime > std::chrono::nanoseconds(2'000'000)) { // 2 ms
                    std::this_thread::sleep_for(sleepTime - std::chrono::nanoseconds(2'000'000));
                }

                while (clock::now() - frameStart < m_FrameDuration) {
                    // busy wait (for high-precision FPS cap)
                }
            }
        }
    }

    vkDeviceWaitIdle(vulkan_vars.device);
}

