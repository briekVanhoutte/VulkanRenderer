#pragma once

#include "Component.h"
#include <iostream>
#include "../../vulkanbase/VulkanUtil.h"

class TransformComponent : public Component {
public:
    TransformComponent()
        : position(0.0f), scale(1.0f), rotation(0.0f) {
    }

    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation;  // Euler angles (could be in degrees or radians)

    void initialize() override {
       
    }

    void update() override {
       
    }

    void render() override {
       
    }
};