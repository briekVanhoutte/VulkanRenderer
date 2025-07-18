#pragma once
#include <glm/glm.hpp>

struct UniformBufferObject {
	glm::mat4 proj;
	glm::mat4 view;
	uint32_t textureID;
};