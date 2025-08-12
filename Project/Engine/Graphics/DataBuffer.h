#pragma once
#include <stdexcept>
#include <cstring>
#include <vulkan/vulkan_core.h>

class DataBuffer
{
public:
    DataBuffer(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkDeviceSize size
    );

    ~DataBuffer() = default;

    // Back-compat names, now zero-cost wrappers over persistent mapping
    void upload(VkDeviceSize size, void* data);
    void uploadRaw(VkDeviceSize size, void* data, VkDeviceSize dstOffset);
    void map(VkDeviceSize size, void* data);      // wrapper over upload()
    void remap(VkDeviceSize size, void* data);    // wrapper over upload()

    void destroy(const VkDevice& device);

    void bindAsVertexBuffer(VkCommandBuffer commandBuffer);
    void bindAsIndexBuffer(VkCommandBuffer commandBuffer);

    VkBuffer getVkBuffer();
    void* getUniformBuffer();                     // returns persistent mapped ptr

    VkDeviceSize getSizeInBytes();
    VkDeviceMemory getBufferMemory() { return m_VkBufferMemory; }

    // NOTE: still sync-submits a tiny one-time CB with a fence.
    // In a perf pass you’d batch this at frame level.
    void copyBuffer(VkBuffer srcBuffer, const VkCommandPool& commandPool,
        const VkDevice& device, VkDeviceSize size, const VkQueue& graphicsQueue);

    static uint32_t findMemoryType(VkPhysicalDevice physicalDevice,
        uint32_t typeFilter, VkMemoryPropertyFlags properties);

private:
    // Helpers
    bool isHostVisible()  const { return (m_Properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0; }
    bool isHostCoherent() const { return (m_Properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0; }

    VkDevice m_VkDevice = VK_NULL_HANDLE;
    VkBuffer m_VkBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_VkBufferMemory = VK_NULL_HANDLE;
    VkDeviceSize m_Size = 0;

    // Persistent mapping
    VkMemoryPropertyFlags m_Properties{};
    void* m_Mapped = nullptr;
    VkDeviceSize m_NonCoherentAtomSize = 0;

    // Back-compat alias (some old code may still read this)
    void* m_UniformBufferMapped = nullptr;
};
