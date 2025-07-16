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

	right = glm::normalize( glm::cross({ 0.f,1.f,0.f }, forward));
	up = glm::cross(forward, right);

	glm::mat4 OBN = { {right,0.f}, { up ,0.f},{ forward ,0.f},{ origin ,0.f} };

	viewMatrix = glm::inverse(OBN);
	invViewMatrix = OBN;
}

void Camera::CalculateProjectionMatrix()
{
	float yScale = 1.0f / tan(fov / 2.0f);
	float xScale = yScale / aspectRatio;
	
	projectionMatrix[0] = glm::vec4(xScale, 0.0f, 0.0f, 0.0f);
	projectionMatrix[1] = glm::vec4(0.0f, yScale, 0.0f, 0.0f);
	projectionMatrix[2] = glm::vec4(0.0f, 0.0f, farPlane / (farPlane - nearPlane), 1.0f);
	projectionMatrix[3] = glm::vec4(0.0f, 0.0f, -nearPlane * farPlane / (farPlane - nearPlane), 0.0f);
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
	glm::mat4 rotationX = CreateRotationX(totalPitch);
	glm::mat4 rotationY = CreateRotationY(totalYaw);

	glm::mat4 finalRotation = rotationX * rotationY;

	forward = glm::vec3(finalRotation * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
}

void Camera::rotate(glm::vec2 offset)
{
	glm::vec2 offsetRadians = glm::radians(offset);

	 totalYaw += offset.x;
	 totalPitch += offset.y;
}

void Camera::translateForward(float posChange)
{
	origin = origin + forward * posChange;
}

void Camera::translateRight(float posChange)
{
	origin = origin + right * posChange;
}



void Camera::SetFov(const float fovAngleNew) {
	fovAngle = fovAngleNew;
	fov = tanf((glm::radians(fovAngle)) / 2.f);
	CalculateProjectionMatrix();
}

void Camera::SetAspectRatio(const float aspect) {
	aspectRatio = aspect;
	CalculateProjectionMatrix();
}
