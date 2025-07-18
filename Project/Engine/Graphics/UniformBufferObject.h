#pragma once
#include <glm/glm.hpp>

struct UniformBufferObject {
	glm::mat4 proj;
	glm::mat4 view;
	glm::vec3 cameraPos;
};