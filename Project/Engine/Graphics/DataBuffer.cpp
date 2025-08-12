#include "DataBuffer.h"
#include <iostream>

static inline VkDeviceSize Align(VkDeviceSize v, VkDeviceSize a) {
    return (v + (a - 1)) & ~(a - 1);
}

DataBuffer::DataBuffer(VkPhysicalDevice physicalDevice, VkDevice device,
    VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
    VkDeviceSize size)
    : m_VkDevice{ device }
    , m_Size{ size }
    , m_Properties{ properties }
{
    VkBufferCreateInfo bufferInfo{ };
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_VkBuffer) != VK_SUCCESS) {
        throw std::runtime_error("DataBuffer: vkCreateBuffer failed");
    }

    VkMemoryRequirements memReq{};
    vkGetBufferMemoryRequirements(device, m_VkBuffer, &memReq);

    VkMemoryAllocateInfo allocInfo{ };
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memReq.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &m_VkBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("DataBuffer: vkAllocateMemory failed");
    }

    if (vkBindBufferMemory(device, m_VkBuffer, m_VkBufferMemory, 0) != VK_SUCCESS) {
        throw std::runtime_error("DataBuffer: vkBindBufferMemory failed");
    }

    // Cache non-coherent atom size
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    m_NonCoherentAtomSize = props.limits.nonCoherentAtomSize;

    // Persistent map for host-visible buffers
    if (isHostVisible()) {
        if (vkMapMemory(m_VkDevice, m_VkBufferMemory, 0, m_Size, 0, &m_Mapped) != VK_SUCCESS) {
            throw std::runtime_error("DataBuffer: vkMapMemory failed");
        }
        // Back-compat: old code may read this
        m_UniformBufferMapped = m_Mapped;
    }
}

void DataBuffer::copyBuffer(VkBuffer srcBuffer, const VkCommandPool& commandPool,
    const VkDevice& device, VkDeviceSize size, const VkQueue& graphicsQueue)
{
    VkCommandBufferAllocateInfo allocInfo{ };
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(device, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{ };
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkBufferCopy copy{ };
    copy.srcOffset = 0;
    copy.dstOffset = 0;
    copy.size = size;
    vkCmdCopyBuffer(cmd, srcBuffer, m_VkBuffer, 1, &copy);

    // Default barrier targets vertex-read; adjust at call site if needed.
    VkBufferMemoryBarrier barrier{ };
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT; // ok for VB; for IB/UBO use a different barrier
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = m_VkBuffer;
    barrier.offset = 0;
    barrier.size = size;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, // adjust if copying to UBO/SSBO/IB
        0,
        0, nullptr,
        1, &barrier,
        0, nullptr
    );

    vkEndCommandBuffer(cmd);

    VkFence fence{};
    VkFenceCreateInfo fi{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    vkCreateFence(device, &fi, nullptr, &fence);

    VkSubmitInfo si{ };
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd;
    vkQueueSubmit(graphicsQueue, 1, &si, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(device, fence, nullptr);
    vkFreeCommandBuffers(device, commandPool, 1, &cmd);
}

void DataBuffer::upload(VkDeviceSize size, void* data)
{
    if (!isHostVisible() || !m_Mapped) {
        throw std::runtime_error("DataBuffer::upload on non-mappable memory; use staging+copyBuffer");
    }
    std::memcpy(m_Mapped, data, static_cast<size_t>(size));

    if (!isHostCoherent()) {
        VkMappedMemoryRange range{ };
        range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory = m_VkBufferMemory;
        range.offset = 0;
        range.size = Align(size, m_NonCoherentAtomSize);
        vkFlushMappedMemoryRanges(m_VkDevice, 1, &range);
    }
    // Keep back-compat alias in sync
    m_UniformBufferMapped = m_Mapped;
}

void DataBuffer::uploadRaw(VkDeviceSize size, void* data, VkDeviceSize dstOffset)
{
    if (!isHostVisible() || !m_Mapped) {
        throw std::runtime_error("DataBuffer::uploadRaw on non-mappable memory; use staging+copyBuffer");
    }
    std::memcpy(static_cast<char*>(m_Mapped) + dstOffset, data, static_cast<size_t>(size));

    if (!isHostCoherent()) {
        // Align offset & size for non-coherent memory
        VkDeviceSize begin = (dstOffset / m_NonCoherentAtomSize) * m_NonCoherentAtomSize;
        VkDeviceSize end = Align(dstOffset + size, m_NonCoherentAtomSize);

        VkMappedMemoryRange range{ };
        range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory = m_VkBufferMemory;
        range.offset = begin;
        range.size = end - begin;
        vkFlushMappedMemoryRanges(m_VkDevice, 1, &range);
    }
    m_UniformBufferMapped = m_Mapped;
}

// Back-compat shims: these used to vkMapMemory every time.
// Now they just copy into the persistent mapping.
void DataBuffer::map(VkDeviceSize size, void* data) { upload(size, data); }
void DataBuffer::remap(VkDeviceSize size, void* data) { upload(size, data); }

void DataBuffer::destroy(const VkDevice& /*device*/)
{
    if (m_Mapped) {
        vkUnmapMemory(m_VkDevice, m_VkBufferMemory);
        m_Mapped = nullptr;
        m_UniformBufferMapped = nullptr;
    }
    if (m_VkBuffer)        vkDestroyBuffer(m_VkDevice, m_VkBuffer, nullptr);
    if (m_VkBufferMemory)  vkFreeMemory(m_VkDevice, m_VkBufferMemory, nullptr);
    m_VkBuffer = VK_NULL_HANDLE;
    m_VkBufferMemory = VK_NULL_HANDLE;
}

void DataBuffer::bindAsVertexBuffer(VkCommandBuffer commandBuffer) {
    VkBuffer bufs[] = { m_VkBuffer };
    VkDeviceSize offs[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, bufs, offs);
}
void DataBuffer::bindAsIndexBuffer(VkCommandBuffer commandBuffer) {
    vkCmdBindIndexBuffer(commandBuffer, m_VkBuffer, 0, VK_INDEX_TYPE_UINT16);
}

VkBuffer DataBuffer::getVkBuffer() { return m_VkBuffer; }
void* DataBuffer::getUniformBuffer() { return m_Mapped; }  // back-compat alias
VkDeviceSize DataBuffer::getSizeInBytes() { return m_Size; }

uint32_t DataBuffer::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProps{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((typeFilter & (1u << i)) &&
            (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("DataBuffer: failed to find suitable memory type");
}
