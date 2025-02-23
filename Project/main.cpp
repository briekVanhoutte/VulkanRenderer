#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32

#include "vulkanbase/VulkanBase.h"
#include "Engine/WindowManager.h"
#include "Engine/InputManager.h"
#include "Engine/RendererManager.h"
#include "Engine/PhysxBase.h"
#include "Engine/SceneModelManager.h"
#include "Engine/GameSceneManager.h"


void initScene(BaseObject* wallObject);

int main() {
	try {
		// Initialize the window.
		WindowManager& winMgr = WindowManager::GetInstance();
		winMgr.initWindow();

		// Initialize input manager with the same window.
		InputManager::GetInstance().Initialize(winMgr.getWindow());
		auto& physics = PhysxBase::GetInstance();
		physics.initPhysics(false);


		Camera* m_Camera = new Camera{};

		float fov{ 90.f };
		float aspectRatio{ float(WIDTH) / float(HEIGHT) };
		glm::vec3 cameraStartLocation{ -4.f,-3.f,-25.f };

		m_Camera->Initialize(fov, cameraStartLocation, aspectRatio);


		RendererManager* renderer = new RendererManager();
		renderer->Initialize();

		auto& sceneManager = SceneModelManager::getInstance();

		BaseObject* m_MovingwallIndex = nullptr;

		initScene(m_MovingwallIndex);

		std::vector<RenderItem> renderItems = std::vector<RenderItem>{};
		renderItems.push_back(RenderItem{ sceneManager.getMeshScene(), 0});
		renderItems.push_back(RenderItem{ sceneManager.getParticleScene(), 1 });

		auto startTime = std::chrono::high_resolution_clock::now();
		float deltaTime = 0.f;
		float totalTime = 0.f;
		int frameCount = 0;
		int fps = 50;

		auto& vulkan_vars = vulkanVars::GetInstance();

		while (!glfwWindowShouldClose(winMgr.getWindow())) {
			auto currentTime = std::chrono::high_resolution_clock::now();
			deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
			startTime = currentTime;

			glfwPollEvents();

			physics.stepPhysics(false);
			renderer->RenderFrame(renderItems, *m_Camera);

			InputManager::GetInstance().HandleCameraInputs(m_Camera, deltaTime);
			m_Camera->update();

			frameCount++;
			totalTime += deltaTime;
			if (totalTime >= 1.0f) {
				fps = frameCount;
				//std::cout << "FPS: " << std::fixed << std::setprecision(2) << fps << std::endl;
				frameCount = 0;
				startTime = std::chrono::high_resolution_clock::now(); // Reset start time
				totalTime = 0.f;
			}
		}
		vkDeviceWaitIdle(vulkan_vars.device);


	}
	catch (const std::exception& e) {
		std::cerr << e.what() << "\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


void initScene(BaseObject* wallObject)
{
    auto& vulkan_vars = vulkanVars::GetInstance();
    auto& physxBase = PhysxBase::GetInstance();

    // --- Particles setup (unchanged) ---
    glm::vec3 posParticles{ 0.f, -10.f, -10.f };
    glm::vec3 scaleParticles{ 1.f, 1.f, 1.f };
    glm::vec3 rotParticles{ 0.f, 0.f, 0.f };

    auto& sceneManager = SceneModelManager::getInstance();
    sceneManager.addParticleGroup(
        physxBase.getParticleBuffer()->getPositionInvMasses(),
        physxBase.getParticleBuffer()->getNbActiveParticles(),
        physxBase.m_Particles,
        posParticles, scaleParticles, rotParticles);

    // Shift the particle group upward.
    posParticles.y += 3.f;

    // --- Compute positions for walls ---
    glm::vec3 posBackWall = posParticles;
    posBackWall.z += 3.f;
    posBackWall.x -= 3.f;

    glm::vec3 posRightWall = posParticles;
    posRightWall.x += physxBase.getRightWallLocation();

    glm::vec3 posLeftWall = posParticles;
    posLeftWall.x += 3.f;

    glm::vec3 posBotWall = posParticles;
    posBotWall.y -= 3.f;
    posBotWall.x -= 3.f;

    // --- Create walls using the GameSceneManager and PrimitiveMeshComponent ---
    auto& gameScene = GameSceneManager::getInstance();

    // Back wall: a plane with width 12 and height 6.
    // To get a wall facing backwards (normal (0,0,-1)) we rotate the default upward plane
    // by -90° about the X-axis.
    GameObject* backWall = gameScene.addGameObject();
    backWall->getTransform()->position = posBackWall;
    backWall->getTransform()->scale = scaleParticles;
    backWall->getTransform()->rotation = glm::vec3(-90.f, 0.f, 0.f);
    backWall->addComponent<PrimitiveMeshComponent>(backWall,PrimitiveType::Plane, 12.f, 6.f, 1.f);

    // Right wall: a plane with width 6 and height 6.
    // For a wall facing right (+X), rotate the plane by -90° about X then -90° about Z.
    GameObject* rightWall = gameScene.addGameObject();
    rightWall->getTransform()->position = posRightWall;
    rightWall->getTransform()->scale = scaleParticles;
    rightWall->getTransform()->rotation = glm::vec3(-90.f, 0.f, -90.f);
    rightWall->addComponent<PrimitiveMeshComponent>(rightWall,PrimitiveType::Plane, 6.f, 6.f, 1.f);

    // Left wall: a plane with width 6 and height 6.
    // For a wall facing left (-X), rotate by -90° about X then +90° about Z.
    GameObject* leftWall = gameScene.addGameObject();
    leftWall->getTransform()->position = posLeftWall;
    leftWall->getTransform()->scale = scaleParticles;
    leftWall->getTransform()->rotation = glm::vec3(-90.f, 0.f, 90.f);
    leftWall->addComponent<PrimitiveMeshComponent>(leftWall,PrimitiveType::Plane, 6.f, 6.f, 1.f);

    // Bottom wall (floor): a plane with width 12 and height 6.
    // The default plane (with no rotation) is assumed horizontal.
    GameObject* bottomWall = gameScene.addGameObject();
    bottomWall->getTransform()->position = posBotWall;
    bottomWall->getTransform()->scale = scaleParticles;
    bottomWall->getTransform()->rotation = glm::vec3(0.f, 0.f, 0.f);
    bottomWall->addComponent<PrimitiveMeshComponent>(bottomWall,PrimitiveType::Plane, 12.f, 6.f, 1.f);

    // Initialize the game scene objects.
    gameScene.initialize();

    // Finally, initialize your SceneModelManager.
    sceneManager.initScenes(vulkan_vars.physicalDevice,
        vulkan_vars.device,
        vulkan_vars.commandPoolModelPipeline.m_CommandPool,
        vulkan_vars.graphicsQueue);
}