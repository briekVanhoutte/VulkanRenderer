#pragma once

#include "Component.h"
#include <glm/glm.hpp>
#include <iostream>

class gameObject; // Forward declaration

class TransformComponent : public Component {
public:
    TransformComponent()
        : position(0.0f), scale(1.0f), rotation(0.0f) {
    }

    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation;

    void initialize() override {
        
    }

    void applyNow();

    void update() override {
        
    }

    void render() override {
        
    }
};