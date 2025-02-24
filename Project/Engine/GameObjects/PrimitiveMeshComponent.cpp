// PrimitiveMeshComponent.cpp
#include "PrimitiveMeshComponent.h"
#include "GameObject.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

void PrimitiveMeshComponent::update()
{
    if (!m_parent) return;

    // Get the parent's transform.
    auto transform = m_parent->getTransform();
    glm::vec3 parentPos = transform->position;

    // Convert Euler angles from degrees to radians before making the quaternion.
    glm::quat parentQuat = glm::quat(glm::radians(transform->rotation));

    // Update each mesh face based on the parent's current transform.
    for (auto& face : m_faceMeshes) {
        // Apply parent's rotation to the stored local offset.
        glm::vec3 updatedPos = parentPos + parentQuat * face.localOffset;
        face.object->setPosition(updatedPos, transform->scale, transform->rotation);
    }
}


void PrimitiveMeshComponent::addPrimitiveToScene(PrimitiveType type, float width, float height, float depth, glm::vec3 color) {
    auto parent = getParent();
    if (!parent) return;

    auto& sceneManager = SceneModelManager::getInstance();
    auto parentTransform = parent->getTransform();

    switch (type) {
    case PrimitiveType::Plane: {
        // For a plane the depth is ignored.
        glm::vec3 normal{ 0.f, 1.f, 0.f };
        glm::vec3 facePos = parentTransform->position;
        BaseObject* obj = sceneManager.addMeshRectangle(normal, color,
            width, height,
            facePos,
            parentTransform->scale,
            parentTransform->rotation);
        // Local offset is zero since it is positioned at the parent's origin.
        m_faceMeshes.push_back({ obj, glm::vec3(0.f) });
        break;
    }
    case PrimitiveType::Cube: {
        glm::vec3 cubeCenter = parentTransform->position;
        glm::vec3 baseRotation = parentTransform->rotation; // expected to be {0,0,0} at creation
        glm::vec3 halfDims(width * 0.5f, height * 0.5f, depth * 0.5f);

        // Front face (normal: (0,0,-1))
        {
            glm::vec3 offset(0.f, 0.f, -halfDims.z);
            glm::vec3 facePos = cubeCenter + offset;
            BaseObject* obj = sceneManager.addMeshRectangle(glm::vec3(0.f, 0.f, -1.f), color,
                width, height,
                facePos,
                parentTransform->scale,
                baseRotation);
            m_faceMeshes.push_back({ obj, offset });
        }
        // Back face (normal: (0,0,1))
        {
            glm::vec3 offset(0.f, 0.f, halfDims.z);
            glm::vec3 facePos = cubeCenter + offset;
            BaseObject* obj = sceneManager.addMeshRectangle(glm::vec3(0.f, 0.f, 1.f), color,
                width, height,
                facePos,
                parentTransform->scale,
                baseRotation);
            m_faceMeshes.push_back({ obj, offset });
        }
        // Right face (normal: (1,0,0))
        {
            glm::vec3 offset(halfDims.x, 0.f, 0.f);
            glm::vec3 facePos = cubeCenter + offset;
            BaseObject* obj = sceneManager.addMeshRectangle(glm::vec3(1.f, 0.f, 0.f), color,
                depth, height,
                facePos,
                parentTransform->scale,
                baseRotation);
            m_faceMeshes.push_back({ obj, offset });
        }
        // Left face (normal: (-1,0,0))
        {
            glm::vec3 offset(-halfDims.x, 0.f, 0.f);
            glm::vec3 facePos = cubeCenter + offset;
            BaseObject* obj = sceneManager.addMeshRectangle(glm::vec3(-1.f, 0.f, 0.f), color,
                depth, height,
                facePos,
                parentTransform->scale,
                baseRotation);
            m_faceMeshes.push_back({ obj, offset });
        }
        // Top face (normal: (0,1,0))
        {
            glm::vec3 offset(0.f, halfDims.y, 0.f);
            glm::vec3 facePos = cubeCenter + offset;
            BaseObject* obj = sceneManager.addMeshRectangle(glm::vec3(0.f, 1.f, 0.f), color,
                width, depth,
                facePos,
                parentTransform->scale,
                baseRotation);
            m_faceMeshes.push_back({ obj, offset });
        }
        // Bottom face (normal: (0,-1,0))
        {
            glm::vec3 offset(0.f, -halfDims.y, 0.f);
            glm::vec3 facePos = cubeCenter + offset;
            BaseObject* obj = sceneManager.addMeshRectangle(glm::vec3(0.f, -1.f, 0.f), color,
                width, depth,
                facePos,
                parentTransform->scale,
                baseRotation);
            m_faceMeshes.push_back({ obj, offset });
        }
        break;
    }
    default:
        break;
    }
}
