#pragma once
#include <cassert>
#include <iostream>
//#include "vulkanbase\VulkanUtil.h"

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/scalar_constants.hpp> // glm::pi

struct Camera
{
	Camera() = default;

	Camera(const glm::vec3& _origin, float _fovAngle, float _aspectRatio);


	glm::vec3 origin{};
	float fovAngle{ 90.f };
	float fov{ glm::radians(fovAngle)};
	float aspectRatio{1.f};

	float nearPlane{ 0.1f };
	float farPlane{ 100.f };

	glm::vec3 forward{ 0.f,0.f,1.f };
	glm::vec3 up{ 0.f,1.f,0.f };
	glm::vec3 right{ 1.f,0.f,0.f };

	float totalPitch{ 3.14f};
	float totalYaw{};

	glm::mat4 invViewMatrix{};
	glm::mat4 viewMatrix{};

	glm::mat4 projectionMatrix{};

	void Initialize(float _fovAngle = 90.f, glm::vec3 _origin = { 0.f,0.f,0.f }, float _aspectRatio = 1.f);

	void CalculateViewMatrix();
	void CalculateProjectionMatrix();

	glm::mat4 CalculateCameraToWorld();
	void printValuesCamera();
	
	void update();

	void rotate(glm::vec2 offset);
	void translateForward(float posChange);
	void translateRight(float posChange);

	void SetFov(const float fovAngleNew);
	void SetAspectRatio(const float aspect);

	glm::mat4 CreateRotationX(float pitch) {
		return glm::rotate(glm::mat4(1.0f), pitch, glm::vec3(1.0f, 0.0f, 0.0f));
	}


	glm::mat4 CreateRotationY(float yaw) {
		return glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::mat4 CreateRotationZ(float roll) {
		return glm::rotate(glm::mat4(1.0f), roll, glm::vec3(0.0f, 0.0f, 1.0f));
	}

};
