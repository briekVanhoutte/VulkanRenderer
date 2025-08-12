#pragma once
#include "Camera.h"


Camera::Camera(const glm::vec3& _origin, float _fovAngle, float _aspectRatio) :
	origin{ _origin },
	fovAngle{ _fovAngle },
	aspectRatio{ _aspectRatio }
{
	fov =  glm::radians(_fovAngle);

	CalculateProjectionMatrix();
}

void Camera::Initialize(float _fovAngle, glm::vec3 _origin, float _aspectRatio)
{
	fovAngle = _fovAngle;
	fov = tanf(glm::radians(fovAngle) / 2.f);

	origin = _origin;

	aspectRatio = _aspectRatio;
}

void Camera::CalculateViewMatrix()
{
	viewMatrix = glm::lookAt(origin, origin + forward, up);
	invViewMatrix = glm::inverse(viewMatrix);
}

void Camera::CalculateProjectionMatrix()
{
	// fov is already in radians here
	projectionMatrix = glm::perspective(fov, aspectRatio, nearPlane, farPlane);
	projectionMatrix[1][1] *= -1.0f;   // Vulkan clip space flip
}
glm::mat4 Camera::CalculateCameraToWorld()
{
	// rightx,   righty,   rightz,   0
	// upx,      upy,      upz,      0
	// forwardx, forwardy, forwardz, 0
	// originx,  originy,  originz,  1

	right = glm::normalize(glm::cross(up, forward));
	right.y = 0; // force no roll
	up = glm::normalize(glm::cross(forward, right)); 


	return { {right ,0.f },
			 { up,0.f},
			 {forward, 0.f },
			 {origin,  1.f }
	};
}

void Camera::printValuesCamera() {
	std::cout << "pitch: " << totalPitch << std::endl;
	std::cout << "yaw: " << totalYaw << std::endl;
	std::cout << "origin: x=" << origin.x << "y= " << origin.y << "z= " << origin.z << std::endl;

	std::cout << "forward: x=" << forward.x << "y= " << forward.y << "z= " << forward.z << std::endl;
	std::cout << "right: x=" << right.x << "y= " << right.y << "z= " << right.z << std::endl;
	std::cout << "up: x=" << up.x << "y= " << up.y << "z= " << up.z << std::endl;
}

void Camera::update()
{

}

void Camera::rotate(glm::vec2 delta) {
	const float sens = -0.004f;
	const float limit = glm::radians(89.0f);

	totalYaw += delta.x * sens;
	totalPitch = glm::clamp(totalPitch + (-delta.y * sens), -limit, limit);

	// Forward from yaw/pitch (right-handed, +Z forward)
	glm::vec3 dir;
	dir.x = cosf(totalPitch) * sinf(totalYaw);
	dir.y = sinf(totalPitch);
	dir.z = cosf(totalPitch) * cosf(totalYaw);
	forward = glm::normalize(dir);

	// IMPORTANT: use this cross order to keep +X right when forward=+Z
	const glm::vec3 worldUp(0, 1, 0);
	right = glm::normalize(glm::cross(worldUp, forward));
	up = glm::normalize(glm::cross(forward, right));
}


void Camera::translateForward(float d) { origin += forward * d; }
void Camera::translateRight(float d) { origin += right * d; }



void Camera::SetFov(const float fovAngleNew) {
	fovAngle = fovAngleNew;
	fov = tanf((glm::radians(fovAngle)) / 2.f);
	CalculateProjectionMatrix();
}

void Camera::SetAspectRatio(const float aspect) {
	aspectRatio = aspect;
	CalculateProjectionMatrix();
}
