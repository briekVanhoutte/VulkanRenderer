#include "Game.h"

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
    // Clean up dynamically allocated memory if necessary.
    delete m_Renderer;
    delete m_Camera;
}

void Game::init() {
    // Initialize the window
    m_WindowManager.initWindow();

    // Initialize the input manager using the window pointer
    InputManager::GetInstance().Initialize(m_WindowManager.getWindow());

    // Initialize physics
    m_Physics.initPhysics(false);

    // Initialize the camera
    float fov = 90.f;
    float aspectRatio = float(WIDTH) / float(HEIGHT);
    glm::vec3 cameraStartLocation{ -4.f, -3.f, -25.f };

    m_Camera = new Camera{};
    m_Camera->Initialize(fov, cameraStartLocation, aspectRatio);

    // Initialize the renderer
    m_Renderer = new RendererManager();
    m_Renderer->Initialize();

    // Setup the scene (walls, particle groups, etc.)
    initScene();

    // Prepare render items (for example, adding mesh and particle scenes)
    m_RenderItems.push_back(RenderItem{ m_SceneManager.getMeshScene(), 0 });
    m_RenderItems.push_back(RenderItem{ m_SceneManager.getParticleScene(), 1 });
}

void Game::initScene() {
    auto& vulkan_vars = vulkanVars::GetInstance();

    // --- Particles setup ---
    glm::vec3 posParticles{ 0.f, -10.f, -10.f };
    glm::vec3 scaleParticles{ 1.f, 1.f, 1.f };
    glm::vec3 rotParticles{ 0.f, 0.f, 0.f };

    //m_SceneManager.addParticleGroup(
    //    m_Physics.getParticleBuffer()->getPositionInvMasses(),
    //    m_Physics.getParticleBuffer()->getNbActiveParticles(),
    //    m_Physics.m_Particles,
    //    posParticles, scaleParticles, rotParticles);

    // Shift the particle group upward.
    posParticles.y += 3.f;

    // --- Compute positions for walls ---
    glm::vec3 posBackWall = posParticles;
    posBackWall.z += 3.f;
    posBackWall.x -= 3.f;

    glm::vec3 posRightWall = posParticles;
    posRightWall.x += m_Physics.getRightWallLocation();

    glm::vec3 posLeftWall = posParticles;
    posLeftWall.x += 3.f;

    glm::vec3 posBotWall = posParticles;
    posBotWall.y -= 3.f;
    posBotWall.x -= 3.f;

    // --- Create walls using the GameSceneManager and PrimitiveMeshComponent ---
    // Back wall: plane with width 12 and height 6. Rotate -90° about X-axis.
    GameObject* backWall = m_GameScene.addGameObject();
    backWall->getTransform()->position = posBackWall;
    backWall->getTransform()->scale = scaleParticles;
    backWall->getTransform()->rotation = glm::vec3(-90.f, 0.f, 0.f);
    backWall->addComponent<PrimitiveMeshComponent>(backWall, PrimitiveType::Plane, 12.f, 6.f, 1.f);

    // Right wall: plane with width 6 and height 6. Rotate -90° about X then -90° about Z.
    GameObject* rightWall = m_GameScene.addGameObject();
    rightWall->getTransform()->position = posRightWall;
    rightWall->getTransform()->scale = scaleParticles;
    rightWall->getTransform()->rotation = glm::vec3(-90.f, 0.f, -90.f);
    rightWall->addComponent<PrimitiveMeshComponent>(rightWall, PrimitiveType::Plane, 6.f, 6.f, 1.f);

    // Left wall: plane with width 6 and height 6. Rotate -90° about X then +90° about Z.
    GameObject* leftWall = m_GameScene.addGameObject();
    leftWall->getTransform()->position = posLeftWall;
    leftWall->getTransform()->scale = scaleParticles;
    leftWall->getTransform()->rotation = glm::vec3(-90.f, 0.f, 90.f);
    leftWall->addComponent<PrimitiveMeshComponent>(leftWall, PrimitiveType::Plane, 6.f, 6.f, 1.f);

    // Bottom wall (floor): plane with width 12 and height 6. No rotation required.
    GameObject* bottomWall = m_GameScene.addGameObject();
    bottomWall->getTransform()->position = posBotWall;
    bottomWall->getTransform()->scale = scaleParticles;
    bottomWall->getTransform()->rotation = glm::vec3(0.f, 0.f, 0.f);
    bottomWall->addComponent<PrimitiveMeshComponent>(bottomWall, PrimitiveType::Plane, 12.f, 6.f, 1.f);

    // Initialize game scene objects.
    m_GameScene.initialize();

    // Initialize SceneModelManager with Vulkan objects.
    m_SceneManager.initScenes(vulkan_vars.physicalDevice,
        vulkan_vars.device,
        vulkan_vars.commandPoolModelPipeline.m_CommandPool,
        vulkan_vars.graphicsQueue);
}

void Game::run() {
    using clock = std::chrono::high_resolution_clock;
    auto lastTime = clock::now();
    float totalTime = 0.f;
    int frameCount = 0;

    while (!glfwWindowShouldClose(m_WindowManager.getWindow())) {
        auto frameStart = clock::now();
        float deltaTime = std::chrono::duration<float>(frameStart - lastTime).count();
        lastTime = frameStart;

        glfwPollEvents();

        // Update physics and render frame.
        m_Physics.stepPhysics(false);
        m_Renderer->RenderFrame(m_RenderItems, *m_Camera);

        // Handle input and update camera.
        InputManager::GetInstance().HandleCameraInputs(m_Camera, deltaTime);
        m_Camera->update();

        // FPS calculation (optional)
        frameCount++;
        totalTime += deltaTime;
        if (totalTime >= 1.0f) {
            std::cout << "FPS: " << std::fixed << std::setprecision(2) << frameCount << std::endl;
            frameCount = 0;
            totalTime = 0.f;
        }

        // Enforce 60 FPS cap
        auto frameEnd = clock::now();
        auto frameElapsed = std::chrono::duration<float>(frameEnd - frameStart);
        if (frameElapsed < m_FrameDuration) {
            std::this_thread::sleep_for(m_FrameDuration - frameElapsed);
        }
    }

    auto& vulkan_vars = vulkanVars::GetInstance();
    vkDeviceWaitIdle(vulkan_vars.device);
}

