#include "ShaderBase.h"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <glm\gtc\type_ptr.hpp>
#include <Engine/Graphics/vulkanVars.h>
#include <Engine/Graphics/MaterialManager.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/TextureManager.h>
#include <algorithm>
#include <Engine/Scene/LineScene.h>
#include <Engine/Scene/MeshScene.h>




ShaderBase::ShaderBase(const std::string& vertexShaderPath, const std::string& fragmentShaderPath)
    :vertexShaderModule_(VK_NULL_HANDLE), fragmentShaderModule_(VK_NULL_HANDLE)
{
    vertexShaderCode_ = readFile(vertexShaderPath);
    fragmentShaderCode_ = readFile(fragmentShaderPath);


    m_Tex = TextureManager::GetInstance().getOrCreateTexture(kErrorTexturePath);
}

void ShaderBase::initialize(const VkPhysicalDevice& vkPhysicalDevice, const VkDevice& vkDevice, const std::vector<VkVertexInputBindingDescription>& bindings, std::vector<VkVertexInputAttributeDescription> attributes)
{
    device_ = vkDevice;

    vertexShaderModule_ = createShaderModule(vertexShaderCode_);
    fragmentShaderModule_ = createShaderModule(fragmentShaderCode_);

    // Keep copies alive for the lifetime of this ShaderBase
    m_VkVertexInputBindingDescs = bindings;
    m_VkVertexInputAttributeDesc = std::move(attributes);

    // Wire up the vertex input state to those member vectors
    m_VertexInputStateInfo = {};
    m_VertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    m_VertexInputStateInfo.vertexBindingDescriptionCount =
        static_cast<uint32_t>(m_VkVertexInputBindingDescs.size());
    m_VertexInputStateInfo.pVertexBindingDescriptions =
        m_VkVertexInputBindingDescs.data();
    m_VertexInputStateInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(m_VkVertexInputAttributeDesc.size());
    m_VertexInputStateInfo.pVertexAttributeDescriptions =
        m_VkVertexInputAttributeDesc.data();

    // --- Your existing UBO / descriptor setup (unchanged) ---
    m_UBOBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        UniformBufferObject ubo{};
        ubo.view = glm::mat4(1.0f);
        ubo.proj = glm::mat4(1.0f);

        m_UBOBuffers[i] = std::make_unique<DataBuffer>(
            vkPhysicalDevice,
            vkDevice,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            sizeof(ubo)
        );
        m_UBOBuffers[i]->upload(sizeof(ubo), &ubo);
    }

    m_DescriptorPool = { vkDevice, sizeof(UniformBufferObject), MAX_FRAMES_IN_FLIGHT };
    m_DescriptorPool.Initialize(vkDevice);

    std::vector<VkBuffer> buffers;
    std::vector<std::vector<VkDescriptorImageInfo>> imagesPerFrame(MAX_FRAMES_IN_FLIGHT);

    const auto& defaultTexture = TextureManager::GetInstance().getStandardTexture();
    auto cachedTextures = TextureManager::GetInstance().getAllCachedTextures(); // copy is fine
    buffers.reserve(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        buffers.push_back(m_UBOBuffers[i]->getVkBuffer());
        imagesPerFrame[i].resize(MAX_TEXTURES);

        for (size_t j = 0; j < MAX_TEXTURES; ++j) {
            if (j < cachedTextures.size() && cachedTextures[j]) {
                imagesPerFrame[i][j] = cachedTextures[j]->getDescriptorInfo();
            }
            else if (defaultTexture) {
                imagesPerFrame[i][j] = defaultTexture->getDescriptorInfo();
            }
            else {
                imagesPerFrame[i][j] = VkDescriptorImageInfo{};
            }
        }
    }

    m_DescriptorPool.createDescriptorSets(buffers, imagesPerFrame);

}

ShaderBase::~ShaderBase() {

}

void ShaderBase::Destroy(const VkDevice& vkDevice)
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_UBOBuffers[i]->destroy(vkDevice);
    }

    m_DescriptorPool.Destroy(vkDevice);

    vkDestroyDescriptorSetLayout(vkDevice, m_DescriptorSetLayout, nullptr);
    
}

void ShaderBase::initialize(const VkPhysicalDevice& vkPhysicalDevice, const VkDevice& vkDevice, const VkVertexInputBindingDescription& vkVertexInputBindingDesc, std::vector<VkVertexInputAttributeDescription>& vkVertexInputAttributeDesc) {
    std::vector<VkVertexInputBindingDescription> bs{ vkVertexInputBindingDesc };
    initialize(vkPhysicalDevice, vkDevice, bs, vkVertexInputAttributeDesc);

}

void ShaderBase::updateDescriptorSet()
{
    auto& vulkan_vars = vulkanVars::GetInstance();
    m_UBOBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        UniformBufferObject ubo{};
        ubo.view = glm::mat4{ {1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1} };
        ubo.proj = glm::mat4{ {1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1} };

        m_UBOBuffers[i] = std::make_unique<DataBuffer>(
            vulkan_vars.physicalDevice,
            vulkan_vars.device,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            sizeof(ubo)
        );

        m_UBOBuffers[i]->upload(sizeof(ubo), &ubo);


    }



    m_DescriptorPool = { vulkan_vars.device, sizeof(UniformBufferObject), MAX_FRAMES_IN_FLIGHT };
    m_DescriptorPool.Initialize(vulkan_vars.device);
    std::vector<VkBuffer> buffers{};
    std::vector<VkDescriptorImageInfo> images{};
    std::vector<std::vector<VkDescriptorImageInfo>> imagesPerFrame(MAX_FRAMES_IN_FLIGHT);

    auto& texmanager = TextureManager::GetInstance();

    const auto& defaultTexture = texmanager.getStandardTexture();
    auto& cachedTextures = texmanager.getAllCachedTextures();

    std::sort(
        cachedTextures.begin(),
        cachedTextures.end(),
        [](const std::shared_ptr<Texture>& a, const std::shared_ptr<Texture>& b) {
            return a->getID() < b->getID();
        }
    );

    size_t numTextures = cachedTextures.size();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        buffers.push_back(m_UBOBuffers[i]->getVkBuffer());

        imagesPerFrame[i].resize(MAX_TEXTURES);
        for (size_t j = 0; j < MAX_TEXTURES; ++j) {
            if (j < cachedTextures.size() && cachedTextures[j] ) {
                imagesPerFrame[i][j] = cachedTextures[j]->getDescriptorInfo();
            }
            else if (defaultTexture) {
                imagesPerFrame[i][j] = defaultTexture->getDescriptorInfo();
            }
            else {
                imagesPerFrame[i][j] = VkDescriptorImageInfo{};
            }
        }
    }

    m_DescriptorPool.createDescriptorSets(buffers, imagesPerFrame);
}

void ShaderBase::createDescriptorSetLayout(const VkDevice& vkDevice)
{
    // Uniform Buffer (binding 0)
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    // Combined Image Sampler (binding 1)
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = MAX_TEXTURES;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding, samplerLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void ShaderBase::bindDescriptorSet(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, size_t index)
{
    m_DescriptorPool.bindDescriptorSet(commandBuffer, pipelineLayout, index);
}

VkPipelineShaderStageCreateInfo ShaderBase::getVertexShaderStageInfo()  {
    VkPipelineShaderStageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    info.module = vertexShaderModule_;
    info.pName = "main";
    return info;
}

VkPipelineShaderStageCreateInfo ShaderBase::getFragmentShaderStageInfo()  {
    VkPipelineShaderStageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    info.module = fragmentShaderModule_;
    info.pName = "main";
    return info;
}

std::vector<char> ShaderBase::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

VkShaderModule ShaderBase::createShaderModule(const std::vector<char>& code) {
    	VkShaderModuleCreateInfo createInfo{};
    	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    	createInfo.codeSize = code.size();
    	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    
    	VkShaderModule shaderModule;
    	if (vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    		throw std::runtime_error("failed to create shader module!");
    	}

        assert(reinterpret_cast<uintptr_t>(code.data()) % 4 == 0 && "SPIR-V code buffer is not 4-byte aligned!");
    
    	return shaderModule;
}

VkPipelineVertexInputStateCreateInfo& ShaderBase::getVertexInputStateInfo() {

    return m_VertexInputStateInfo;
}


VkPipelineInputAssemblyStateCreateInfo& ShaderBase::getInputAssemblyStateInfo(VkPrimitiveTopology topology)  {

    m_VkPipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_VkPipelineInputAssemblyStateCreateInfo.topology = topology;
    m_VkPipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
    return m_VkPipelineInputAssemblyStateCreateInfo;
}

void ShaderBase::updateUniformBuffer(uint32_t currentImage, UniformBufferObject& ubo) {
    if (m_UBOBuffers.size() > currentImage) {
        m_UBOBuffers[currentImage]->remap(sizeof(ubo), &ubo);
    }   
}


