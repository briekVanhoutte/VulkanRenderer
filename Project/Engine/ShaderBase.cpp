#include "ShaderBase.h"
#include <fstream>
#include <stdexcept>
#include "vulkanbase\VulkanUtil.h"
#include <iostream>
#include <glm\gtc\type_ptr.hpp>

// Constructor
ShaderBase::ShaderBase(const std::string& vertexShaderPath, const std::string& fragmentShaderPath)
    :vertexShaderModule_(VK_NULL_HANDLE), fragmentShaderModule_(VK_NULL_HANDLE)
{
    vertexShaderCode_ = readFile(vertexShaderPath);
    fragmentShaderCode_ = readFile(fragmentShaderPath);
}

// Destructor
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

// Initialize shader modules
void ShaderBase::initialize(const VkPhysicalDevice& vkPhysicalDevice, const VkDevice& vkDevice) {
    device_ = vkDevice;

    vertexShaderModule_ = createShaderModule(vertexShaderCode_);
    fragmentShaderModule_ = createShaderModule(fragmentShaderCode_);
    m_VkVertexInputBindingDesc = Vertex::getBindingDescription();
    m_VkVertexInputAttributeDesc = Vertex::getAttributeDescriptions();

    m_UBOBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        UniformBufferObject ubo{};
        //ubo.model = glm::mat4{ {1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1} };
        ubo.view = glm::mat4{ {1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1} };
        ubo.proj = glm::mat4{ {1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1} };

        m_UBOBuffers[i] = std::make_unique<DataBuffer>(
            vkPhysicalDevice,
            vkDevice,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            sizeof(ubo)
        );

        m_UBOBuffers[i]->upload(sizeof(ubo),&ubo);
    }



    m_DescriptorPool = { vkDevice, sizeof(UniformBufferObject), MAX_FRAMES_IN_FLIGHT };
    m_DescriptorPool.Initialize(vkDevice);
    std::vector<VkBuffer> buffers{};
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        buffers.push_back(m_UBOBuffers[i]->getVkBuffer());
    }

    m_DescriptorPool.createDescriptorSets(buffers);

}

void ShaderBase::createDescriptorSetLayout(const VkDevice& vkDevice)
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void ShaderBase::bindDescriptorSet(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, size_t index)
{
    m_DescriptorPool.bindDescriptorSet(commandBuffer, pipelineLayout, index);
}

// Get Vertex Shader Stage Info
VkPipelineShaderStageCreateInfo ShaderBase::getVertexShaderStageInfo()  {
    VkPipelineShaderStageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    info.module = vertexShaderModule_;
    info.pName = "main"; // Entry point
    return info;
}

// Get Fragment Shader Stage Info
VkPipelineShaderStageCreateInfo ShaderBase::getFragmentShaderStageInfo()  {
    VkPipelineShaderStageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    info.module = fragmentShaderModule_;
    info.pName = "main"; // Entry point
    return info;
}

// Private Helper: Create a shader module from bytecode
VkShaderModule ShaderBase::createShaderModule(const std::vector<char>& code) {
    	VkShaderModuleCreateInfo createInfo{};
    	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    	createInfo.codeSize = code.size();
    	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    
    	VkShaderModule shaderModule;
    	if (vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    		throw std::runtime_error("failed to create shader module!");
    	}
    
    	return shaderModule;
}

VkPipelineVertexInputStateCreateInfo ShaderBase::getVertexInputStateInfo() {
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &m_VkVertexInputBindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_VkVertexInputAttributeDesc.size());
    vertexInputInfo.pVertexAttributeDescriptions = m_VkVertexInputAttributeDesc.data();

    return vertexInputInfo;
}


VkPipelineInputAssemblyStateCreateInfo ShaderBase::getInputAssemblyStateInfo()  {
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;
	return inputAssembly;
}

void ShaderBase::updateUniformBuffer(uint32_t currentImage, UniformBufferObject& ubo) {
    if (m_UBOBuffers.size() > currentImage) {
        m_UBOBuffers[currentImage]->remap(sizeof(ubo), &ubo);
    }   
}