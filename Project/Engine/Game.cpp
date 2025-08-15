#include "Game.h"
#include <Engine/Graphics/vulkanVars.h>
#include "Platform/Windows/PlatformWindow_Windows.h"
#include <Engine/Graphics/MaterialManager.h>
#include <vector>
#include <random>

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
    glm::vec3 cameraStartLocation{ 0, 3, -25.f };

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
    const glm::vec3 camPos = { 0, -3, -25 };          // e.g. {-4, -3, -25}
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
    std::shared_ptr<Material> catMat = std::make_shared<Material>("Resources/Textures/Cat/Cat_diffuse.jpg", "", "", "");
    std::shared_ptr<Material> kdhMat = std::make_shared<Material>("Resources/Textures/kdh.jpg");
    std::shared_ptr<Material> errorMat = std::make_shared<Material>();

    // List used for random selection. Put 3 here if you only want 3.
    std::vector<std::shared_ptr<Material>> cubeMats = {
         testMat, kdhMat
    };

    // RNG (seed once)
    static std::mt19937 rng{ std::random_device{}() };

    // --- 3) Big ground plane centered under the camera -------------------------
    {
        GameObject* ground = m_GameScene.addGameObject();
        ground->getTransform()->position = glm::vec3(camPos.x,0.f, camPos.z);
        ground->getTransform()->scale = glm::vec3(1.f);
        ground->getTransform()->rotation = glm::vec3(0.f, 0.f, 0.f);
        ground->addComponent<PrimitiveMeshComponent>(
            ground, PrimitiveType::Plane, 200.f, 200.f, 1.f, stoneMat,true);
    }

    // --- Sprinkle some cats ------------------------------------------------------
    {
        // Tunables
        const int   maxCats = 20;      // <= max amount
        const float areaR = 10.f;    // place cats in [-areaR, +areaR] on X/Z
        const float baseY = 0.f;     // height to place them at
        const glm::vec3 baseRot = glm::vec3(90, 90.f, 180.f); // your original pitch

        // "scaling var const"
        const float catScaleMin = 0.1f;
        const float catScaleMax = 0.1f;

        // RNG bits (uses your existing rng; if you don't have one, make: std::mt19937 rng{std::random_device{}()};)
        std::uniform_real_distribution<float> posOff(-areaR, areaR);
        std::uniform_real_distribution<float> jitter(-0.10f, 0.10f);
        std::uniform_real_distribution<float> scaleDist(catScaleMin, catScaleMax);

        int placed = 0;
        while (placed < maxCats) {
            const float x = posOff(rng);
            const float z = posOff(rng);
            const float s = scaleDist(rng);
            const float yaw = -90;

            GameObject* cat = m_GameScene.addGameObject();
            cat->getTransform()->position = glm::vec3(x + jitter(rng), baseY, z + jitter(rng));
            cat->getTransform()->scale = glm::vec3(s);                    // uniform scale
            cat->getTransform()->rotation = baseRot + glm::vec3(0.f, yaw, 0.f); // keep -90° pitch, random yaw

            cat->addComponent<ModelMeshComponent>(cat, "Resources/Models/cat.obj", catMat, false);
            ++placed;
        }
    }

    // --- Tiny randomized stacked cubes ------------------------------------------
    {
        const int   maxCubes = 100;
        const int   columns = 1800;
        const float areaR = 10.f;
        const float baseY = 0.f;
        const glm::vec3 center(0, baseY, 0);

        std::uniform_real_distribution<float> posOff(-areaR, areaR);
        std::uniform_real_distribution<float> jitter(-0.03f, 0.03f);
        std::uniform_int_distribution<int>    stackCount(3, 12);
        std::uniform_real_distribution<float> sizeDist(1.f, 1.f);
        std::uniform_int_distribution<int>    rotPick(0, 3);
        std::uniform_int_distribution<size_t> matPick(0, cubeMats.empty() ? 0 : cubeMats.size() - 1);

        int placed = 0;
        for (int c = 0; c < columns && placed < maxCubes; ++c) {
            float x = center.x + posOff(rng);
            float z = center.z + posOff(rng);
            int   stacks = stackCount(rng);

            float yCursor = baseY;
            for (int k = 0; k < stacks && placed < maxCubes; ++k) {
                float w = sizeDist(rng);
                float h = sizeDist(rng);
                float d = sizeDist(rng);

                GameObject* cube = m_GameScene.addGameObject();
                cube->getTransform()->position = glm::vec3(x + jitter(rng), yCursor + 0.5f * h, z + jitter(rng));
                cube->getTransform()->scale = glm::vec3(1.f);
                cube->getTransform()->rotation = glm::vec3(0.f, 90.f * float(rotPick(rng)), 0.f);

                std::shared_ptr<Material> mat = cubeMats.empty() ? errorMat : cubeMats[matPick(rng)];
                cube->addComponent<PrimitiveMeshComponent>(cube, PrimitiveType::Cube, w, h, d, mat);

                yCursor += h;
                ++placed;
            }
        }
    }

    // --- 5) Backdrop wall placed in front of the camera and facing it ----------
    {
        GameObject* wall = m_GameScene.addGameObject();
        const float wallDist = 60.f;                 // distance in front of camera
        const float wallHeight = 10.f;                 // raise wall center a bit
        wall->getTransform()->position = glm::vec3{0.f,0.f,1.f} * wallDist;

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

    auto& sm = SceneModelManager::getInstance();
    //sm.getMeshScene()->debugPrintVisibleBatches(std::cout);   // what will be drawn this frame
    //sm.getMeshScene()->getChunkGrid().debugPrintStorage(std::cout); // if you expose getChunkGrid()
    //sm.getMeshScene()->getChunkGrid().debugPrintObjectPlacement(std::cout);
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

