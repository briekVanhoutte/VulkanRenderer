#pragma once

#include "Component.h"
#include "../SceneModelManager.h"
#include <glm/glm.hpp>
#include <iostream>

enum class PrimitiveType {
    Plane,
    Cube
    // Extend with additional primitives (Sphere, etc.) as needed.
};

class PrimitiveMeshComponent : public Component {
public:
    // Constructor takes the parent GameObject, a primitive type, and dimensions.
    PrimitiveMeshComponent(GameObject* parent, PrimitiveType type, float width = 1.f, float height = 1.f, float depth = 1.f) {
        setParent(parent);
        addPrimitiveToScene(type, width, height, depth);
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
    void addPrimitiveToScene(PrimitiveType type, float width, float height, float depth);
};
