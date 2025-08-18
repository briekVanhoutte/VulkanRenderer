#pragma once

#include "Component.h"
#include "../SceneModelManager.h"
#include <glm/glm.hpp>
#include <iostream>

enum class PrimitiveType {
    Plane,
    Cube
};

struct FaceInfo {
    BaseObject* bo = nullptr;
    glm::vec3   localNormal{ 0 };   // ±X/±Y/±Z in cube local
    glm::vec3   localCenter{ 0 };   // offset from cube center in cube local (±w/2,0,0 etc.)
    float       baseW = 0.f;      // face width before parent scale
    float       baseH = 0.f;      // face height before parent scale
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
    const std::vector<BaseObject*>& getBaseObjects() const { return m_bases; }
    BaseObject* getFirstBase() const { return m_bases.empty() ? nullptr : m_bases[0]; }

    void onTransformUpdated(const glm::vec3& pos,
        const glm::vec3& scale,
        const glm::vec3& rot) override;

private:
    // Updated to accept dimensions.
    void addPrimitiveToScene(PrimitiveType type, float width, float height, float depth, const std::shared_ptr<Material> mat = {});
    bool m_makeGlobal = false;
    std::vector<BaseObject*> m_bases;

    std::vector<FaceInfo> m_faces;     // only used for cubes
    glm::vec3 m_cubeDims{ 1,1,1 };       // (width,height,depth) that were passed in
};
