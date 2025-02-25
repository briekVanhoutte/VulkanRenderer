#pragma once

#include "Component.h"
#include "../SceneModelManager.h"
#include "../../vulkanbase/VulkanUtil.h"
#include <iostream>

enum class PrimitiveType {
    Plane,
    Cube
    // Extend with additional primitives (Sphere, etc.) as needed.
};

struct FaceMeshUpdateData {
    BaseObject* object;
    glm::vec3 localOffset;
};


class PrimitiveMeshComponent : public Component {
public:
    // Constructor takes the parent GameObject, a primitive type, and dimensions.
    PrimitiveMeshComponent(GameObject* parent, PrimitiveType type, float width = 1.f, float height = 1.f, float depth = 1.f, glm::vec3 color = {}) {
        setParent(parent);
        addPrimitiveToScene(type, width, height, depth, color);
    }

    void initialize() override {
        
    }
    void update() override;
    void render() override {
     
    }

private:
    // Updated to accept dimensions.
    void addPrimitiveToScene(PrimitiveType type, float width, float height, float depth, glm::vec3 color);
    std::vector<FaceMeshUpdateData> m_faceMeshes;

};
