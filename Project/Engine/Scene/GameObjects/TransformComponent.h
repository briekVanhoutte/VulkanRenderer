#pragma once

#include "Component.h"
#include <glm/glm.hpp>
#include <iostream>

class TransformComponent : public Component {
public:
    TransformComponent()
        : position(0.0f), scale(1.0f), rotation(0.0f) {
    }

    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation;

    void initialize() override {
        std::cout << "TransformComponent initialized." << std::endl;
    }

    void update() override {
        std::cout << "TransformComponent updated." << std::endl;
    }

    void render() override {
        std::cout << "TransformComponent rendered." << std::endl;
    }
};