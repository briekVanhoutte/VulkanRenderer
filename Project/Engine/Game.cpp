#include "Game.h"
#include "GameObjects/GameObject.h"
#include <cstdlib>
#include <ctime>
#include "vulkanVars.h"


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
    m_PhysicsManager.initialize();

    // Initialize the camera.
    float fov = 90.f;
    float aspectRatio = float(WIDTH) / float(HEIGHT);
    glm::vec3 cameraStartLocation{ 0.f,2.f, -20.f };

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
    // Seed the random number generator.
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // -----------------------------------
    // Create the Floor
    // -----------------------------------
    // Create a GameObject for the floor.
    GameObject* floor = m_GameScene.addGameObject();
    // Position the floor at y = -1.
    floor->getTransform()->position = glm::vec3(0.f, 0.f, -10.f);
    // Optionally, set a large scale (for the mesh) to simulate a big floor.
    floor->getTransform()->scale = glm::vec3(5.f, 5.f, 1.f);

    // Add a plane collider component. Here we assume the floor's plane has an upward normal (0,1,0)
    // and is defined at d = 1 (since the plane equation is n·p + d = 0, and for a floor at y=-1, d=+1).
    floor->addComponent<PlaneColliderComponent>(PxVec3(0, 1, 0), 1.f);

    // Add a visual mesh component for the floor.
    // Assuming PrimitiveType::Plane exists and the mesh scales with the GameObject's scale.
    glm::vec3 floorColor(0.3f, 0.3f, 0.3f); // gray-ish color
    floor->addComponent<PrimitiveMeshComponent>(floor, PrimitiveType::Plane, 5.f, 5.f, 1.f, floorColor);

    // -----------------------------------
    // Create a random number of cubes
    // -----------------------------------
    // Choose a random count between 10 and 20 cubes.
    int cubeCount = 10 + (rand() % 11); // random number in [10,20]

    for (int i = 0; i < cubeCount; ++i) {
        // Create a new GameObject for the cube.
        GameObject* cube = m_GameScene.addGameObject();

        // Randomize position above the floor (y between 2 and 10), and x/z within a region.
        float posX = -10.f + static_cast<float>(rand()) / RAND_MAX * 20.f;
        float posY = 5.f + static_cast<float>(rand()) / RAND_MAX * 8.f;
        float posZ = -10.f + static_cast<float>(rand()) / RAND_MAX * 20.f;
        cube->getTransform()->position = glm::vec3(posX, posY, posZ);

        // Generate a random uniform cube size between 0.5 and 2.5 units.
        float size = 0.5f + (static_cast<float>(rand()) / RAND_MAX) * 2.0f;

        // Add a box collider component (with dynamic set to true so it acts as a rigid body).
        cube->addComponent<BoxColliderComponent>(size, size, size, true);

        // Generate a random color for the cube.
        glm::vec3 color(
            static_cast<float>(rand()) / RAND_MAX,
            static_cast<float>(rand()) / RAND_MAX,
            static_cast<float>(rand()) / RAND_MAX
        );

        cube->addComponent<RigidBodyComponent>();

        // Add a visual mesh component for the cube.
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
        m_PhysicsManager.update(deltaTime);

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
            //std::cout << "FPS: " << std::fixed << std::setprecision(2) << frameCount << std::endl;
            frameCount = 0;
            totalTime = 0.f;
        }

        // Cap the FPS: Sleep for the remaining frame time if necessary.
        auto frameEnd = clock::now();
        auto frameTime = std::chrono::duration<float>(frameEnd - frameStart);
        if (frameTime < m_FrameDuration) {
            std::this_thread::sleep_for(m_FrameDuration - frameTime);
        }
    }

    auto& vulkan_vars = vulkanVars::GetInstance();
    vkDeviceWaitIdle(vulkan_vars.device);
}
