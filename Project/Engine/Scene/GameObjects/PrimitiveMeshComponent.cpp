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
        BaseObject* obj = sceneManager.addMeshRectangle(normal, color, width, height,
            parent->getTransform()->position,
            parent->getTransform()->scale,
            parent->getTransform()->rotation,
            mat);
        if (m_makeGlobal) {
            sceneManager.getMeshScene()->setObjectGlobal(obj, true);
        }
        break;
    }
    case PrimitiveType::Cube: {
        auto& sceneManager = SceneModelManager::getInstance();
        MeshScene* ms = sceneManager.getMeshScene();

        glm::vec3 C = parent->getTransform()->position;
        glm::vec3 Rdeg = parent->getTransform()->rotation;
        glm::vec3 S = parent->getTransform()->scale;

        // Rotation + scaled half-sizes
        glm::quat q = glm::quat(glm::radians(Rdeg));
        float W = width * S.x;
        float H = height * S.y;
        float D = depth * S.z;

        // AABB half-extents after rotation (abs(R) * halfSize)
        auto aabbHalf = [&](const glm::quat& q, glm::vec3 half) {
            glm::mat3 R = glm::mat3_cast(q);
            glm::vec3 ax = glm::abs(glm::vec3(R[0])); // column 0
            glm::vec3 ay = glm::abs(glm::vec3(R[1])); // column 1
            glm::vec3 az = glm::abs(glm::vec3(R[2])); // column 2
            return glm::vec3(
                ax.x * half.x + ay.x * half.y + az.x * half.z,
                ax.y * half.x + ay.y * half.y + az.y * half.z,
                ax.z * half.x + ay.z * half.y + az.z * half.z
            );
            };
        glm::vec3 halfBase(W * 0.5f, H * 0.5f, D * 0.5f);
        glm::vec3 he = aabbHalf(q, halfBase); // group coverage half-extents

        glm::vec3 offX = q * glm::vec3(0.5f * W, 0.0f, 0.0f);
        glm::vec3 offY = q * glm::vec3(0.0f, 0.5f * H, 0.0f);
        glm::vec3 offZ = q * glm::vec3(0.0f, 0.0f, 0.5f * D);

        auto face = [&](const glm::vec3& nrmLocal, const glm::vec3& centerOff,
            float faceW, float faceH)
            {
                BaseObject* obj = sceneManager.addMeshRectangle(
                    nrmLocal,
                    glm::vec3(1.f, 0.f, 1.f),
                    faceW, faceH,
                    C + centerOff,
                    glm::vec3(1.f), // unit scale (we baked it into W/H/D)
                    Rdeg,
                    mat
                );
                // Key line: give this face the cube’s group AABB
                ms->setObjectCoverageOverride(obj, /*center*/ C, /*halfExtents*/ he);
                if (m_makeGlobal) { ms->setObjectGlobal(obj, true); } // if you kept the global flag
            };

        // Z faces (front/back)
        face(glm::vec3(0, 0, 1), offZ, W, H);
        face(glm::vec3(0, 0, -1), -offZ, W, H);
        // X faces (left/right)
        face(glm::vec3(1, 0, 0), offX, D, H);
        face(glm::vec3(-1, 0, 0), -offX, D, H);
        // Y faces (top/bottom)
        face(glm::vec3(0, 1, 0), offY, W, D);
        face(glm::vec3(0, -1, 0), -offY, W, D);
        break;
    }
    default:
        break;
    }
}
