//#include "vulkanbase/VulkanBase.h"
//
//void VulkanBase::initWindow() {
//	glfwInit();
//	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
//	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
//}


#include "vulkanbase/VulkanBase.h"
void VulkanBase::initWindow() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
		void* pUser = glfwGetWindowUserPointer(window);
		VulkanBase* vBase = static_cast<VulkanBase*>(pUser);
		vBase->keyEvent(key, scancode, action, mods);
		});
	glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
		void* pUser = glfwGetWindowUserPointer(window);
		VulkanBase* vBase = static_cast<VulkanBase*>(pUser);
		vBase->mouseMove(window, xpos, ypos);
		});
	glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
		void* pUser = glfwGetWindowUserPointer(window);
		VulkanBase* vBase = static_cast<VulkanBase*>(pUser);
		vBase->mouseEvent(window, button, action, mods);
		});


}
void VulkanBase::keyEvent(int key, int scancode, int action, int mods)
{

	if (action == GLFW_PRESS) {
		if (std::find(keysDown.begin(), keysDown.end(), key) == keysDown.end()) {
			keysDown.push_back(key);
		}
	}
	if (action == GLFW_RELEASE) {
		auto it = std::find(keysDown.begin(), keysDown.end(), key);
		if (it != keysDown.end()) {
			keysDown.erase(it);
		}
	}
}
void VulkanBase::mouseMove(GLFWwindow* window, double xpos, double ypos)
{
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
	if (state == GLFW_PRESS)
	{
		float dx = static_cast<float>(xpos) - m_LastMousePos.x;
		if (dx > 0) {
			m_Rotation += 0.01;
		}
		else {
			m_Rotation -= 0.01;
		}
	}
}

void VulkanBase::mouseEvent(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS) {
		if (std::find(mouseDown.begin(), mouseDown.end(), button) == mouseDown.end()) {
			mouseDown.push_back(button);
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
			glm::vec2 newPos{};
			newPos.x = static_cast<float>(xpos);
			newPos.y = static_cast<float>(ypos);
			m_LastMousePos = newPos;
		}
	}
	if (action == GLFW_RELEASE) {
		auto it = std::find(mouseDown.begin(), mouseDown.end(), button);
		if (it != mouseDown.end()) {
			mouseDown.erase(it);
		}
	}
}

void VulkanBase::HandleKeyInputs(float deltaTime) {

	const float MoveSpeed = deltaTime * 10000.f;

	if (std::find(keysDown.begin(), keysDown.end(), GLFW_KEY_W) != keysDown.end()) {
		m_Camera.translateForward(-MoveSpeed);
	}

	if (std::find(keysDown.begin(), keysDown.end(), GLFW_KEY_S) != keysDown.end()) {
		m_Camera.translateForward(MoveSpeed);
	}

	if (std::find(keysDown.begin(), keysDown.end(), GLFW_KEY_D) != keysDown.end()) {
		m_Camera.translateRight(MoveSpeed);
	}

	if (std::find(keysDown.begin(), keysDown.end(), GLFW_KEY_A) != keysDown.end()) {
		m_Camera.translateRight(-MoveSpeed);
	}
}

void VulkanBase::HandleMouseInputs(float deltaTime)
{
	const float MoveSpeed = deltaTime * 100.f;

	if (std::find(mouseDown.begin(), mouseDown.end(), GLFW_MOUSE_BUTTON_RIGHT) != mouseDown.end()) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		glm::vec2 newPos{};
		newPos.x = static_cast<float>(xpos);
		newPos.y = static_cast<float>(ypos);

		glm::vec2 offset = { newPos - m_LastMousePos};
		offset.x *= MoveSpeed;
		offset.y *= MoveSpeed;

		if (newPos != m_LastMousePos) {
			m_Camera.rotate(offset);

			m_LastMousePos = newPos;
			//std::cout << "mouse.x: " << offset.x << " mouse.y: " << offset.y << std::endl;
		}

	}

}
