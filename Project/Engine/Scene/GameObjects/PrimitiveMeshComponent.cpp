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
        auto& sceneManager = SceneModelManager::getInstance();

        glm::vec3 C = parent->getTransform()->position;
        glm::vec3 Rdeg = parent->getTransform()->rotation;
        glm::vec3 S = parent->getTransform()->scale;

        glm::quat q = glm::quat(glm::radians(Rdeg)); // rotate offsets

        // final face sizes after scale
        float W = width * S.x;
        float H = height * S.y;
        float D = depth * S.z;

        // rotated, scaled half-offsets from cube center
        glm::vec3 offX = q * glm::vec3(0.5f * W, 0.0f, 0.0f);
        glm::vec3 offY = q * glm::vec3(0.0f, 0.5f * H, 0.0f);
        glm::vec3 offZ = q * glm::vec3(0.0f, 0.0f, 0.5f * D);

        auto face = [&](const glm::vec3& nrmLocal, const glm::vec3& centerOff, float faceW, float faceH) {
            sceneManager.addMeshRectangle(
                nrmLocal,                       // local axis; rotation Rdeg will orient it
                glm::vec3(1.f, 0.f, 1.f),      // color (your choice)
                faceW, faceH,                  // already scaled sizes for this face
                C + centerOff,                 // rotated, scaled center
                glm::vec3(1.f),                // unit scale to avoid double scaling
                Rdeg,                          // cube rotation applied uniformly
                mat
            );
            };

        // Z faces (front/back): width=W, height=H
        face(glm::vec3(0, 0, 1), offZ, W, H);
        face(glm::vec3(0, 0, -1), -offZ, W, H);

        // X faces (left/right): width across Z => D, height=H
        face(glm::vec3(1, 0, 0), offX, D, H);
        face(glm::vec3(-1, 0, 0), -offX, D, H);

        // Y faces (top/bottom): width=W, height across Z => D
        face(glm::vec3(0, 1, 0), offY, W, D);
        face(glm::vec3(0, -1, 0), -offY, W, D);
        break;
    }
    default:
        break;
    }
}
