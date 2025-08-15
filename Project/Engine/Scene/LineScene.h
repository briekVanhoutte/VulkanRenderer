// LineScene.h
#pragma once
#include <Engine/Scene/Scene.h>
#include <Engine/Graphics/DataBuffer.h>
#include <Engine/Graphics/DebugLineVertex.h>
#include <Engine/Graphics/vulkanVars.h>

class LineScene : public Scene {
public:
    void clear() { m_vertices.clear(); }
    void addLine(const glm::vec3& a, const glm::vec3& b, uint32_t rgba) {
        m_vertices.push_back({ a, rgba });
        m_vertices.push_back({ b, rgba });
    }
    // convenience
    void addRect(const glm::vec3& a, const glm::vec3& b, float y, uint32_t rgba) {
        // a = (minX, ?, minZ), b = (maxX, ?, maxZ)
        glm::vec3 p0(a.x, y, a.z), p1(b.x, y, a.z), p2(b.x, y, b.z), p3(a.x, y, b.z);
        addLine(p0, p1, rgba); addLine(p1, p2, rgba); addLine(p2, p3, rgba); addLine(p3, p0, rgba);
    }

    void drawScene(VkPipelineLayout&, VkCommandBuffer& cmd) override {
        auto& vk = vulkanVars::GetInstance();
        size_t frame = vk.currentFrame % MAX_FRAMES_IN_FLIGHT;

        ensureVB(frame, m_vertices.size());
        if (!m_vertices.empty())
            m_vb[frame]->upload(sizeof(DebugLineVertex) * m_vertices.size(), m_vertices.data());

        // Optional thickness (requires VK_FEATURE wideLines on device)
        vkCmdSetLineWidth(cmd, m_lineWidth);

        VkBuffer buf = m_vb[frame]->getVkBuffer();
        VkDeviceSize off = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &buf, &off);
        vkCmdDraw(cmd, (uint32_t)m_vertices.size(), 1, 0, 0);
    }

    void deleteScene(VkDevice device) override {
        for (auto& b : m_vb) {
            if (b) b->destroy(device);  // your buffer’s own destroy, if needed
            b.reset();                  // release the unique_ptr
        }
        m_vertices.clear();
    }

    void setLineWidth(float w) { m_lineWidth = w; }

private:
    void ensureVB(size_t frame, size_t vcount) {
        VkDeviceSize need = (VkDeviceSize)(vcount * sizeof(DebugLineVertex));
        if (!m_vb[frame] || m_vb[frame]->getSizeInBytes() < need) {
            VkDeviceSize newSize = std::max<VkDeviceSize>(need, m_vb[frame] ? m_vb[frame]->getSizeInBytes() * 2 : 64 * 1024);
            auto& vk = vulkanVars::GetInstance();
            m_vb[frame] = std::make_unique<DataBuffer>(
                vk.physicalDevice, vk.device,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                newSize
            );
        }
    }

    std::vector<DebugLineVertex> m_vertices;
    std::array<std::unique_ptr<DataBuffer>, MAX_FRAMES_IN_FLIGHT> m_vb{};
    float m_lineWidth = 1.f;
};
