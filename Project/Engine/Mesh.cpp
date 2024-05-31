#include "Mesh.h"
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/scalar_constants.hpp> // glm::pi

//DataBuffer::DataBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkDeviceSize size)


Mesh::Mesh(const std::vector<Vertex>& Vertexes, const std::vector<uint16_t>& indices)
	:m_Vertices(Vertexes), m_Indices(indices)
{
	m_VertexConstant = {};
	m_VertexConstant.model = glm::mat4{ {1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1} };
}

void Mesh::initialize(VkPhysicalDevice physicalDevice, VkDevice device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue) {
	// create vertex buffer
	CreateVertexBuffer(physicalDevice, device, commandPool, graphicsQueue);

	CreateIndexBuffer(physicalDevice, device, commandPool, graphicsQueue);
	// create indice buffer
}


void Mesh::CreateVertexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue) {
	VkDeviceSize vertexBufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	auto VertexStagingBuffer = std::make_unique<DataBuffer>(physicalDevice, device,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		vertexBufferSize
	);

	VertexStagingBuffer->map(vertexBufferSize, m_Vertices.data());

	m_VertexBuffer = std::make_unique<DataBuffer>(physicalDevice, device,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertexBufferSize
	);


	m_VertexBuffer->copyBuffer(VertexStagingBuffer->getVkBuffer(), commandPool, device, vertexBufferSize, graphicsQueue);
	VertexStagingBuffer->destroy(device);
}
void Mesh::CreateIndexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue) {
	VkDeviceSize indexBufferSize = sizeof(m_Indices[0]) * m_Indices.size();

	auto VertexStagingBuffer = std::make_unique<DataBuffer>(physicalDevice, device,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		indexBufferSize
	);

	VertexStagingBuffer->map(indexBufferSize, m_Indices.data());

	m_IndexBuffer = std::make_unique<DataBuffer>(physicalDevice, device,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		sizeof(m_Indices[0]) * m_Indices.size()
	);

	m_IndexBuffer->copyBuffer(VertexStagingBuffer->getVkBuffer(), commandPool, device, indexBufferSize,graphicsQueue);
	VertexStagingBuffer->destroy(device);
}

void Mesh::destroyMesh(const VkDevice& device) {
	m_VertexBuffer->destroy(device);
	m_IndexBuffer->destroy(device);
}

void Mesh::setPosition(glm::vec3 position, glm::vec3 scale, glm::vec3 rotationAngles)
{
	// Translation matrix for position
	glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);

	// Rotation matrix for rotation angles
	glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngles.x), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotationAngles.y), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotationAngles.z), glm::vec3(0.0f, 0.0f, 1.0f));

	// Scale matrix for scaling
	glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

	// Combine the transformations
	m_VertexConstant.model = translationMatrix * rotationMatrix * scaleMatrix;
}

void Mesh::draw(VkPipelineLayout pipelineLayout, VkCommandBuffer commandBuffer) {
	m_VertexBuffer->bindAsVertexBuffer(commandBuffer);
	m_IndexBuffer->bindAsIndexBuffer(commandBuffer);

	vkCmdPushConstants(
		commandBuffer,
		pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT, // Stage flag should match the push constant range in the layout
		0,                          // Offset within the push constant block
		sizeof(MeshData),          // Size of the push constants to update
		&m_VertexConstant                  // Pointer to the data
	);

	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_Indices.size()), 1, 0, 0, 0);
}

void Mesh::addVertex(glm::vec3 pos, glm::vec3 color, glm::vec3 normal) {
	Vertex newVertex{};
	newVertex.pos = pos;
	newVertex.color = color;
	newVertex.normal = normal;

	m_Vertices.push_back(newVertex);
}

void Mesh::addTriangle(uint16_t i1, uint16_t i2, uint16_t i3, uint16_t offset)
{
	m_Indices.push_back(i1 + offset * 3);
	m_Indices.push_back(i2 + offset * 3);
	m_Indices.push_back(i3 + offset * 3);
}
