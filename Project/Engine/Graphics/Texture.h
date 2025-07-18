#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <atomic>

namespace {
    const std::string kErrorTexturePath = "Resources/Textures/errorTexture.jpg";
}

class Texture {
    friend class TextureManager;
public:
   
    ~Texture();

    VkImageView getImageView() const { return m_ImageView; }
    VkSampler getSampler() const { return m_Sampler; }
    VkDescriptorImageInfo getDescriptorInfo() const;

    uint32_t getID();
private:
    Texture(const std::string& filename);

    void createTextureImage(const std::string& filename);
    void createTextureImageView();
    void createTextureSampler();
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    VkImage m_Image = VK_NULL_HANDLE;
    VkDeviceMemory m_ImageMemory = VK_NULL_HANDLE;
    VkImageView m_ImageView = VK_NULL_HANDLE;
    VkSampler m_Sampler = VK_NULL_HANDLE;
    uint32_t m_Width = 0, m_Height = 0;
    uint32_t m_ID = -1;

    static std::atomic<uint32_t> s_NextID;
};
