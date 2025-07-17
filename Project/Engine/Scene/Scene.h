#pragma once
#include <Engine/Graphics/ShaderBase.h>
class Scene {
public:

    virtual void drawScene(VkPipelineLayout& pipelineLayout, VkCommandBuffer& buffer) = 0;
    virtual  void deleteScene(VkDevice device) = 0;
    
};

