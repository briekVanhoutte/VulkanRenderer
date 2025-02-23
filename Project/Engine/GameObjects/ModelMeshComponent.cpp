#include "ModelMeshComponent.h"
#include "../SceneModelManager.h"
#include "GameObject.h"

void ModelMeshComponent::addModelToScene(const std::string& modelFile) {
    auto parent = getParent();
    if (!parent) return;

    glm::vec3 color{ 1.f, 0.f, 1.f };
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;

    // Parse the model file to fill vertices and indices.
    ParseOBJ(modelFile, vertices, indices, color);

    auto& sceneManager = SceneModelManager::getInstance();
    sceneManager.addMeshModel(vertices, indices,
        parent->getTransform()->position,
        parent->getTransform()->scale,
        parent->getTransform()->rotation);
}