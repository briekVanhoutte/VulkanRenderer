#include "PrimitiveMeshComponent.h"
#include "GameObject.h"

void PrimitiveMeshComponent::addPrimitiveToScene(PrimitiveType type, float width, float height, float depth) {
    auto parent = getParent();
    if (!parent) return;

    glm::vec3 color{ 1.f, 0.f, 1.f };
    auto& sceneManager = SceneModelManager::getInstance();

    switch (type) {
    case PrimitiveType::Plane: {
        // For a plane, use width and height (depth is ignored).
        glm::vec3 normal{ 0.f, 1.f, 0.f };
        sceneManager.addMeshRectangle(normal, color, width, height,
            parent->getTransform()->position,
            parent->getTransform()->scale,
            parent->getTransform()->rotation);
        break;
    }
    case PrimitiveType::Cube: {
        // For a cube, create 6 faces using the provided dimensions.
        glm::vec3 cubeCenter = parent->getTransform()->position;
        glm::vec3 baseRotation = parent->getTransform()->rotation;
        // Calculate half-dimensions.
        glm::vec3 halfDims(width * 0.5f, height * 0.5f, depth * 0.5f);

        // Front face (normal: 0, 0, 1) uses width x height.
        {
            glm::vec3 facePosition = cubeCenter + glm::vec3(0.f, 0.f, halfDims.z);
            glm::vec3 faceRotation = baseRotation; // no extra rotation
            glm::vec3 faceNormal = glm::vec3(0.f, 0.f, 1.f);
            sceneManager.addMeshRectangle(faceNormal, color, width, height,
                facePosition, parent->getTransform()->scale, faceRotation);
        }
        // Back face (normal: 0, 0, -1)
        {
            glm::vec3 facePosition = cubeCenter + glm::vec3(0.f, 0.f, -halfDims.z);
            glm::vec3 faceRotation = baseRotation + glm::vec3(0.f, 180.f, 0.f);
            glm::vec3 faceNormal = glm::vec3(0.f, 0.f, -1.f);
            sceneManager.addMeshRectangle(faceNormal, color, width, height,
                facePosition, parent->getTransform()->scale, faceRotation);
        }
        // Right face (normal: 1, 0, 0) uses depth x height.
        {
            glm::vec3 facePosition = cubeCenter + glm::vec3(halfDims.x, 0.f, 0.f);
            glm::vec3 faceRotation = baseRotation + glm::vec3(0.f, 90.f, 0.f);
            glm::vec3 faceNormal = glm::vec3(1.f, 0.f, 0.f);
            sceneManager.addMeshRectangle(faceNormal, color, depth, height,
                facePosition, parent->getTransform()->scale, faceRotation);
        }
        // Left face (normal: -1, 0, 0)
        {
            glm::vec3 facePosition = cubeCenter + glm::vec3(-halfDims.x, 0.f, 0.f);
            glm::vec3 faceRotation = baseRotation + glm::vec3(0.f, -90.f, 0.f);
            glm::vec3 faceNormal = glm::vec3(-1.f, 0.f, 0.f);
            sceneManager.addMeshRectangle(faceNormal, color, depth, height,
                facePosition, parent->getTransform()->scale, faceRotation);
        }
        // Top face (normal: 0, 1, 0) uses width x depth.
        {
            glm::vec3 facePosition = cubeCenter + glm::vec3(0.f, halfDims.y, 0.f);
            glm::vec3 faceRotation = baseRotation + glm::vec3(-90.f, 0.f, 0.f);
            glm::vec3 faceNormal = glm::vec3(0.f, 1.f, 0.f);
            sceneManager.addMeshRectangle(faceNormal, color, width, depth,
                facePosition, parent->getTransform()->scale, faceRotation);
        }
        // Bottom face (normal: 0, -1, 0)
        {
            glm::vec3 facePosition = cubeCenter + glm::vec3(0.f, -halfDims.y, 0.f);
            glm::vec3 faceRotation = baseRotation + glm::vec3(90.f, 0.f, 0.f);
            glm::vec3 faceNormal = glm::vec3(0.f, -1.f, 0.f);
            sceneManager.addMeshRectangle(faceNormal, color, width, depth,
                facePosition, parent->getTransform()->scale, faceRotation);
        }
        break;
    }
    default:
        break;
    }
}
