#pragma once

#include <vulkan\vulkan_core.h>
#include <Engine/Graphics/Vertex.h>
#include <glm/glm.hpp>
#include <Engine/Graphics/DataBuffer.h>
#include <Engine/Scene/MeshData.h>
#include <Engine/Graphics/MaterialManager.h>
#include <memory>
class Mesh {
public:
	Mesh(const std::vector<Vertex>& Vertexes, const std::vector<uint16_t>& indices,const std::string& TexturePath="");
	void initialize(VkPhysicalDevice physicalDevice, VkDevice device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue);
	void destroyMesh(const VkDevice& device);

	void setPosition(glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles);
	glm::vec3 getPostion() { return m_Position; }
	void addVertex(glm::vec3 pos, glm::vec3 color, glm::vec3 normal);
	void addTriangle(uint16_t i1, uint16_t i2, uint16_t i3, uint16_t offset = 0);

	void draw(VkPipelineLayout pipelineLayout, VkCommandBuffer commandBuffer);
private:
	std::vector<Vertex> m_Vertices;
	std::vector<uint16_t> m_Indices;
	std::unique_ptr<DataBuffer> m_VertexBuffer;
	std::unique_ptr<DataBuffer> m_IndexBuffer;

	MeshData m_VertexConstant;

	glm::vec3 m_Position = {};
	std::shared_ptr<Material> m_Material;
	void CreateVertexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue);
	void CreateIndexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue);
}; 