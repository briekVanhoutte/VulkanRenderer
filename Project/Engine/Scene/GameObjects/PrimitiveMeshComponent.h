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
    PrimitiveMeshComponent(GameObject* parent, PrimitiveType type, float width = 1.f, float height = 1.f, float depth = 1.f, const std::shared_ptr<Material> mat = {}, bool makeGlobal = false)
        :m_makeGlobal(makeGlobal)
    {
        setParent(parent);
        addPrimitiveToScene(type, width, height, depth, mat);
    }

    void initialize() override {
        
    }
    void update() override {
        
    }
    void render() override {
        
    }

private:
    // Updated to accept dimensions.
    void addPrimitiveToScene(PrimitiveType type, float width, float height, float depth, const std::shared_ptr<Material> mat = {});
    bool m_makeGlobal = false;
};
