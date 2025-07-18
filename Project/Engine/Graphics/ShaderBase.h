#pragma once
#ifndef SHADERBASE_H
#define SHADERBASE_H

#include <vulkan/vulkan.h>
#include <Engine/Graphics/Vertex.h>
#include <vector>
#include <string>
#include <memory>
#include <array>
#include <Engine/Graphics/DataBuffer.h>
#include <Engine/Graphics/DescriptorPool.h>
#include <Engine/Graphics/UniformBufferObject.h>

#include <memory>
#include <Engine/Graphics/Texture.h>

class ShaderBase {
public:
    ShaderBase() {};
    ShaderBase( const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
    ~ShaderBase();

    void Destroy(const VkDevice& vkDevice);

    void initialize(const VkPhysicalDevice& vkPhysicalDevice, const VkDevice& vkDevice, const VkVertexInputBindingDescription& vkVertexInputBindingDesc, std::vector<VkVertexInputAttributeDescription>& vkVertexInputAttributeDesc);

    void createDescriptorSetLayout(const VkDevice& vkDevice);
    const VkDescriptorSetLayout& getDescriptorSetLayout()
    {
        return m_DescriptorSetLayout;
    }
    void bindDescriptorSet(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, size_t index);

    void updateUniformBuffer(uint32_t currentImage, UniformBufferObject& ubo);
    void updateDescriptorSet();
    VkPipelineShaderStageCreateInfo getVertexShaderStageInfo() ;
    VkPipelineShaderStageCreateInfo getFragmentShaderStageInfo() ;
    VkPipelineVertexInputStateCreateInfo& getVertexInputStateInfo() ;
    VkPipelineInputAssemblyStateCreateInfo& getInputAssemblyStateInfo(VkPrimitiveTopology topology) ;

private:
    VkDevice device_{};
    std::vector<char> vertexShaderCode_{};
    std::vector<char> fragmentShaderCode_{};

    VkVertexInputBindingDescription m_VkVertexInputBindingDesc{};
    VkPipelineInputAssemblyStateCreateInfo m_VkPipelineInputAssemblyStateCreateInfo{};
    std::vector<VkVertexInputAttributeDescription> m_VkVertexInputAttributeDesc{};
       
    VkShaderModule vertexShaderModule_{};
    VkShaderModule fragmentShaderModule_{};

    VkPipelineVertexInputStateCreateInfo m_VertexInputStateInfo{};

    VkDescriptorSetLayout m_DescriptorSetLayout{};
    std::vector<std::unique_ptr<DataBuffer>> m_UBOBuffers{};

    UniformBufferObject m_UBOSrc{};
    DescriptorPool m_DescriptorPool{};


    std::vector<char> readFile(const std::string& filename);
    VkShaderModule createShaderModule(const std::vector<char>& code);

    //test
    std::shared_ptr<Texture> m_Tex;
};

#endif
