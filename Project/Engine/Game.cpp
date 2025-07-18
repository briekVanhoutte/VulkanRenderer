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

    glm::vec3 posParticles{ 0.f, -10.f, -10.f };
    glm::vec3 scaleParticles{ 1.f, 1.f, 1.f };
    glm::vec3 rotParticles{ 0.f, 0.f, 0.f };

    auto* particleBuffer = m_Physics.getParticleBuffer();

    if (particleBuffer) {
        // Optionally check that positionInvMasses is valid and nbActiveParticles > 0
        auto* positions = particleBuffer->getPositionInvMasses();
        int nbActive = particleBuffer->getNbActiveParticles();

        if (positions && nbActive > 0) {
            m_SceneManager.addParticleGroup(
                positions,
                nbActive,
                m_Physics.getParticles(),
                posParticles, scaleParticles, rotParticles);
        }
        else {
            // You can log or handle the error case here
            // std::cout << "Particle buffer is empty or invalid!\n";
        }
    }
    else {
        // std::cout << "No particle buffer available!\n";
    }
    posParticles.y += 3.f;

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

    std::shared_ptr<Material> stoneMat = std::make_shared<Material>("Resources/Textures/Rocks/rocks_albedo.jpg", "Resources/Textures/Rocks/rocks_normal.jpg", "", "Resources/Textures/Rocks/rocks_roughness.jpg", "Resources/Textures/Rocks/rocks_displacement.jpg");
    std::shared_ptr<Material> bronMat = std::make_shared<Material>("Resources/Textures/errorTexture.jpg");
    std::shared_ptr<Material> serraMat = std::make_shared<Material>("Resources/Textures/testTexture.jpg");
    std::shared_ptr<Material> errorMat = std::make_shared<Material>();


    GameObject* backWall = m_GameScene.addGameObject();
    backWall->getTransform()->position = posBackWall;
    backWall->getTransform()->scale = scaleParticles;
    backWall->getTransform()->rotation = glm::vec3(-90.f, 0.f, 0.f);
    backWall->addComponent<PrimitiveMeshComponent>(backWall, PrimitiveType::Plane, 12.f, 6.f, 1.f, stoneMat);

    GameObject* rightWall = m_GameScene.addGameObject();
    rightWall->getTransform()->position = posRightWall;
    rightWall->getTransform()->scale = scaleParticles;
    rightWall->getTransform()->rotation = glm::vec3(-90.f, 0.f, 90.f);
    rightWall->addComponent<PrimitiveMeshComponent>(rightWall, PrimitiveType::Plane, 6.f, 6.f, 1.f, bronMat);

    GameObject* leftWall = m_GameScene.addGameObject();
    leftWall->getTransform()->position = posLeftWall;
    leftWall->getTransform()->scale = scaleParticles;
    leftWall->getTransform()->rotation = glm::vec3(-90.f, 0.f, 90.f);
    leftWall->addComponent<PrimitiveMeshComponent>(leftWall, PrimitiveType::Plane, 6.f, 6.f, 1.f, serraMat);

    GameObject* bottomWall = m_GameScene.addGameObject();
    bottomWall->getTransform()->position = posBotWall;
    bottomWall->getTransform()->scale = scaleParticles;
    bottomWall->getTransform()->rotation = glm::vec3(0.f, 0.f, 0.f);
    bottomWall->addComponent<PrimitiveMeshComponent>(bottomWall, PrimitiveType::Plane, 12.f, 6.f, 1.f, serraMat);

    m_GameScene.initialize();

    m_SceneManager.initScenes(vulkan_vars.physicalDevice,
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

