#pragma once

#include "Component.h"
#include "../SceneModelManager.h"
#include <glm/glm.hpp>
#include <iostream>

enum class PrimitiveType {
    Plane,
    Cube
};

class PrimitiveMeshComponent : public Component {
public:
    PrimitiveMeshComponent(GameObject* parent, PrimitiveType type, float width = 1.f, float height = 1.f, float depth = 1.f, const std::string& filePath = "") {
        setParent(parent);
        addPrimitiveToScene(type, width, height, depth,filePath);
    }

    void initialize() override {
        std::cout << "PrimitiveMeshComponent initialized." << std::endl;
    }
    void update() override {
        std::cout << "PrimitiveMeshComponent updated." << std::endl;
    }
    void render() override {
        std::cout << "PrimitiveMeshComponent rendered." << std::endl;
    }

private:
    // Updated to accept dimensions.
    void addPrimitiveToScene(PrimitiveType type, float width, float height, float depth, const std::string& filePath = "");
};
