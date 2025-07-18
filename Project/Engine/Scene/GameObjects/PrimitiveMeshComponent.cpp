#include "PrimitiveMeshComponent.h"
#include "GameObject.h"

void PrimitiveMeshComponent::addPrimitiveToScene(PrimitiveType type, float width, float height, float depth, const std::shared_ptr<Material> mat) {
    auto parent = getParent();
    if (!parent) return;

    glm::vec3 color{ 1.f, 0.f, 1.f };
    auto& sceneManager = SceneModelManager::getInstance();

    switch (type) {
    case PrimitiveType::Plane: {
        glm::vec3 normal{ 0.f, 1.f, 0.f };
        sceneManager.addMeshRectangle(normal, color, width, height,
            parent->getTransform()->position,
            parent->getTransform()->scale,
            parent->getTransform()->rotation,
            mat);
        break;
    }
    case PrimitiveType::Cube: {
        glm::vec3 cubeCenter = parent->getTransform()->position;
        glm::vec3 baseRotation = parent->getTransform()->rotation;
        glm::vec3 halfDims(width * 0.5f, height * 0.5f, depth * 0.5f);
        {
            glm::vec3 facePosition = cubeCenter + glm::vec3(0.f, 0.f, halfDims.z);
            glm::vec3 faceRotation = baseRotation;
            glm::vec3 faceNormal = glm::vec3(0.f, 0.f, 1.f);
            sceneManager.addMeshRectangle(faceNormal, color, width, height,
                facePosition, parent->getTransform()->scale, faceRotation, mat);
        }
        {
            glm::vec3 facePosition = cubeCenter + glm::vec3(0.f, 0.f, -halfDims.z);
            glm::vec3 faceRotation = baseRotation + glm::vec3(0.f, 180.f, 0.f);
            glm::vec3 faceNormal = glm::vec3(0.f, 0.f, -1.f);
            sceneManager.addMeshRectangle(faceNormal, color, width, height,
                facePosition, parent->getTransform()->scale, faceRotation, mat);
        }
        {
            glm::vec3 facePosition = cubeCenter + glm::vec3(halfDims.x, 0.f, 0.f);
            glm::vec3 faceRotation = baseRotation + glm::vec3(0.f, 90.f, 0.f);
            glm::vec3 faceNormal = glm::vec3(1.f, 0.f, 0.f);
            sceneManager.addMeshRectangle(faceNormal, color, depth, height,
                facePosition, parent->getTransform()->scale, faceRotation, mat);
        }
        {
            glm::vec3 facePosition = cubeCenter + glm::vec3(-halfDims.x, 0.f, 0.f);
            glm::vec3 faceRotation = baseRotation + glm::vec3(0.f, -90.f, 0.f);
            glm::vec3 faceNormal = glm::vec3(-1.f, 0.f, 0.f);
            sceneManager.addMeshRectangle(faceNormal, color, depth, height,
                facePosition, parent->getTransform()->scale, faceRotation, mat);
        }
        {
            glm::vec3 facePosition = cubeCenter + glm::vec3(0.f, halfDims.y, 0.f);
            glm::vec3 faceRotation = baseRotation + glm::vec3(-90.f, 0.f, 0.f);
            glm::vec3 faceNormal = glm::vec3(0.f, 1.f, 0.f);
            sceneManager.addMeshRectangle(faceNormal, color, width, depth,
                facePosition, parent->getTransform()->scale, faceRotation, mat);
        }
        {
            glm::vec3 facePosition = cubeCenter + glm::vec3(0.f, -halfDims.y, 0.f);
            glm::vec3 faceRotation = baseRotation + glm::vec3(90.f, 0.f, 0.f);
            glm::vec3 faceNormal = glm::vec3(0.f, -1.f, 0.f);
            sceneManager.addMeshRectangle(faceNormal, color, width, depth,
                facePosition, parent->getTransform()->scale, faceRotation, mat);
        }
        break;
    }
    default:
        break;
    }
}
