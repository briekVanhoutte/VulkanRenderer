#pragma once

#include <vulkan\vulkan_core.h>
#include <Engine/Graphics/Vertex.h>
#include <glm/glm.hpp>
#include <Engine/Graphics/DataBuffer.h>
#include <Engine/Graphics/MeshData.h>
#include <Engine/Graphics/MaterialManager.h>
#include <memory>
class Mesh {
public:
	Mesh(const std::vector<Vertex>& Vertexes, const std::vector<uint32_t>& indices, const std::shared_ptr<Material> mat = {});
	void initialize(VkPhysicalDevice physicalDevice, VkDevice device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue);
	void destroyMesh(const VkDevice& device);

	void setPosition(glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles);
	glm::vec3 getPostion() { return m_Position; }
	void addVertex(glm::vec3 pos, glm::vec3 color, glm::vec3 normal, glm::vec2 uv);
	void addTriangle(uint32_t i1, uint32_t i2, uint32_t i3, uint32_t offset = 0);

	VkBuffer getVertexBuffer() const {
		// Replace getBuffer() with your DataBuffer accessor name if different
		return m_VertexBuffer ? m_VertexBuffer->getVkBuffer() : VK_NULL_HANDLE;
	}
	VkBuffer getIndexBuffer() const {
		return m_IndexBuffer ? m_IndexBuffer->getVkBuffer() : VK_NULL_HANDLE;
	}
	VkDeviceSize getVBOffset() const { return 0; } // if DataBuffer tracks offsets, return it here
	VkDeviceSize getIBOffset() const { return 0; }

	uint32_t getIndexCount() const { return static_cast<uint32_t>(m_Indices.size()); }

	const std::shared_ptr<Material>& getMaterial() const { return m_Material; }

	bool isInitialized() const { return m_VertexBuffer && m_IndexBuffer; }

	// Optional (handy later):
	const std::vector<Vertex>& cpuVertices() const { return m_Vertices; }
	const std::vector<uint32_t>& cpuIndices() const { return m_Indices; }
	void draw(VkPipelineLayout pipelineLayout, VkCommandBuffer commandBuffer);

	const glm::mat4& getModelMatrix() const { return m_VertexConstant.model; }

	void setMaterial(std::shared_ptr<Material> mat) { m_Material = mat; }
private:
	std::vector<Vertex> m_Vertices;
	std::vector<uint32_t> m_Indices;
	std::unique_ptr<DataBuffer> m_VertexBuffer;
	std::unique_ptr<DataBuffer> m_IndexBuffer;

	MeshData m_VertexConstant;

	glm::vec3 m_Position = {};
	std::shared_ptr<Material> m_Material;
	void CreateVertexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue);
	void CreateIndexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue);
}; 