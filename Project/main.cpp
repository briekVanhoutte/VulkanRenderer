#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32

#include "vulkanbase/VulkanBase.h"
#include "Engine/WindowManager.h"
#include "Engine/InputManager.h"
#include "Engine/RendererManager.h"
#include "Engine/PhysxBase.h"
void initScene(MeshScene* scene1, ParticleScene* scene2, unsigned int& movingwallIndex);

int main() {
	try {
		// Initialize the window.
		WindowManager& winMgr = WindowManager::GetInstance();
		winMgr.initWindow();

		// Initialize input manager with the same window.
		InputManager::GetInstance().Initialize(winMgr.getWindow());
		auto& physics = PhysxBase::GetInstance();
		physics.initPhysics(false);


		Camera m_Camera = Camera{};

		float fov{ 90.f };
		float aspectRatio{ float(WIDTH) / float(HEIGHT) };
		glm::vec3 cameraStartLocation{ -4.f,-3.f,-25.f };

		m_Camera.Initialize(fov, cameraStartLocation, aspectRatio);


		RendererManager* renderer = new RendererManager();
		renderer->Initialize();

		MeshScene* m_Scene = new MeshScene{};
		ParticleScene* m_Scene2 = new ParticleScene{};
		unsigned int m_MovingwallIndex;

		initScene(m_Scene, m_Scene2, m_MovingwallIndex);

		std::vector<RenderItem> renderItems = std::vector<RenderItem>{};
		renderItems.push_back(RenderItem{ m_Scene, 0 });
		renderItems.push_back(RenderItem{ m_Scene2, 1 });

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
			renderer->RenderFrame(renderItems, m_Camera);

			m_Camera.update();

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


void initScene(MeshScene* scene1, ParticleScene* scene2, unsigned int& movingwallIndex)
{
	auto& vulkan_vars = vulkanVars::GetInstance();
	auto& physxBase = PhysxBase::GetInstance();

	glm::vec3 posParticles{ 0.f,-10.f,-10.f };
	glm::vec3 scaleParticles{ 1.f,1.f,1.f };
	glm::vec3 rotParticles{ 0.f,0.f,0.f };

	scene2->addParticleGroup(physxBase.getParticleBuffer()->getPositionInvMasses(), physxBase.getParticleBuffer()->getNbActiveParticles(), physxBase.m_Particles, posParticles, scaleParticles, rotParticles);

	// create container for particles

	posParticles.y += 3.f;

	// back
	glm::vec3 posBackWall{ posParticles };
	posBackWall.z += 3.f;
	posBackWall.x -= 3.f;
	scene1->addRectangle({ 0.f, 0.f, -1.f }, { 0.f,0.9f,0.f }, 12.f, 6.f, posBackWall, scaleParticles, rotParticles);

	auto& physx_base = PhysxBase::GetInstance();

	// right
	glm::vec3 posLeftWall{ posParticles };
	posLeftWall.x += physx_base.getRightWallLocation();
	movingwallIndex = scene1->addRectangle({ 1.f, 0.f, 0.f }, { 0.f,0.9f,0.f }, 6.f, 6.f, posLeftWall, scaleParticles, rotParticles);

	// left
	glm::vec3 posRightWall{ posParticles };
	posRightWall.x += 3.f;
	scene1->addRectangle({ -1.f, 0.f, 0.f }, { 0.f,0.9f,0.f }, 6.f, 6.f, posRightWall, scaleParticles, rotParticles);

	// bot
	glm::vec3 posBotWall{ posParticles };
	posBotWall.y -= 3.f;
	posBotWall.x -= 3.f;
	scene1->addRectangle({ 0.f, 1.f, 0.f }, { 0.f,0.9f,0.f }, 12.f, 6.f, posBotWall, scaleParticles, rotParticles);


	scene1->initObject(vulkan_vars.physicalDevice, vulkan_vars.device, vulkan_vars.commandPoolModelPipeline.m_CommandPool, vulkan_vars.graphicsQueue);
}