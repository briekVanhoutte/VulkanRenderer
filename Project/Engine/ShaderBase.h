#pragma once
#ifndef SHADERBASE_H
#define SHADERBASE_H

#include <vulkan/vulkan.h>
#include "vulkanbase\VulkanUtil.h"
#include <vector>
#include <string>
#include <memory>
#include <array>
#include "Engine\DataBuffer.h"
#include "Engine\DescriptorPool.h"

class ShaderBase {
public:
    // Constructor and Destructor
    ShaderBase() {};
    ShaderBase( const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
    ~ShaderBase();

    void Destroy(const VkDevice& vkDevice);

    // Public interface for ShaderBase
    void initialize(const VkPhysicalDevice& vkPhysicalDevice, const VkDevice& vkDevice, const VkVertexInputBindingDescription& vkVertexInputBindingDesc, std::vector<VkVertexInputAttributeDescription>& vkVertexInputAttributeDesc);

    void createDescriptorSetLayout(const VkDevice& vkDevice);
    const VkDescriptorSetLayout& getDescriptorSetLayout()
    {
        return m_DescriptorSetLayout;
    }
    void bindDescriptorSet(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, size_t index);

    void updateUniformBuffer(uint32_t currentImage, UniformBufferObject& ubo);

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



    VkShaderModule createShaderModule(const std::vector<char>& code);
};

#endif // SHADERBASE_H
