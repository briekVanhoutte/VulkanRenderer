#include "Game.h"
#include "GameObjects/GameObject.h"
#include <cstdlib>
#include <ctime>

Game::Game()
    : m_WindowManager(WindowManager::GetInstance()),
    m_SceneManager(SceneModelManager::getInstance()),
    m_GameScene(GameSceneManager::getInstance()),
    m_Renderer(nullptr),
    m_Camera(nullptr),
    m_PhysicsManager(PhysicsManager::getInstance())
{
}

Game::~Game() {
    // Clean up dynamically allocated memory if necessary.
    delete m_Renderer;
    delete m_Camera;
}

void Game::init() {
    // Initialize the window.
    m_WindowManager.initWindow();

    // Initialize the input manager using the window pointer.
    InputManager::GetInstance().Initialize(m_WindowManager.getWindow());

    // Initialize physics.
   /* m_Physics.initPhysics(false);*/

    // Initialize the camera.
    float fov = 90.f;
    float aspectRatio = float(WIDTH) / float(HEIGHT);
    glm::vec3 cameraStartLocation{ 0.f,3.f, -25.f };

    m_Camera = new Camera{};
    m_Camera->Initialize(fov, cameraStartLocation, aspectRatio);

    // Initialize the renderer.
    m_Renderer = new RendererManager();
    m_Renderer->Initialize();

    // Setup the scene: only a ground plane and a falling cube.
    initScene();

    // Prepare render items (for example, adding mesh and particle scenes).
    m_RenderItems.push_back(RenderItem{ m_SceneManager.getMeshScene(), 0 });
   // m_RenderItems.push_back(RenderItem{ m_SceneManager.getParticleScene(), 1 });

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(vulkanVars::GetInstance().physicalDevice, &deviceProperties);
    std::cout << "Using GPU: " << deviceProperties.deviceName << std::endl;
}

void Game::initScene() {
    // Seed the random number generator (only once ideally)
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // Loop to create 100 cubes.
    for (int i = 0; i < 500; ++i) {
        // Create a new GameObject for the cube.
        GameObject* cube = m_GameScene.addGameObject();

        // Random position within a 10x10x10 region.
        float posX = static_cast<float>(rand()) / RAND_MAX * 10.0f;
        float posY = static_cast<float>(rand()) / RAND_MAX * 10.0f;
        float posZ = static_cast<float>(rand()) / RAND_MAX * 10.0f;
        cube->getTransform()->position = glm::vec3(posX, posY, posZ);

        // Generate a random uniform size between 0.5 and 2.5 units.
        float size = 0.5f + (static_cast<float>(rand()) / RAND_MAX) * 2.0f;

        // Add a box collider component with the same dimensions as the cube.
        cube->addComponent<BoxColliderComponent>(size, size, size, false);

        // Generate a random color (each channel between 0.0 and 1.0).
        glm::vec3 color(
            static_cast<float>(rand()) / RAND_MAX,
            static_cast<float>(rand()) / RAND_MAX,
            static_cast<float>(rand()) / RAND_MAX
        );

        // Add a visual mesh component for a cube, using the random size and color.
        // Ensure that your PrimitiveMeshComponent is updated to accept the color parameter.
        cube->addComponent<PrimitiveMeshComponent>(cube, PrimitiveType::Cube, size, size, size, color);
    }

    // Finalize scene initialization.
    m_GameScene.initialize();

    // Initialize SceneModelManager with Vulkan objects.
    auto& vulkan_vars = vulkanVars::GetInstance();
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

        // Update physics.
        //m_PhysicsManager.update(deltaTime);

        m_GameScene.update();

        // Render the frame.
        m_Renderer->RenderFrame(m_RenderItems, *m_Camera);

        // Handle input and update camera.
        InputManager::GetInstance().HandleCameraInputs(m_Camera, deltaTime);
        m_Camera->update();

        // FPS calculation (optional).
        frameCount++;
        totalTime += deltaTime;
        if (totalTime >= 1.0f) {
            std::cout << "FPS: " << std::fixed << std::setprecision(2) << frameCount << std::endl;
            frameCount = 0;
            totalTime = 0.f;
        }
    }

    auto& vulkan_vars = vulkanVars::GetInstance();
    vkDeviceWaitIdle(vulkan_vars.device);
}
