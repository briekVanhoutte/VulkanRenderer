#include "ModelMeshComponent.h"
#include "../SceneModelManager.h"
#include "GameObject.h"
#include <Engine/ObjUtils/ObjUtils.h>
#include <cctype>
#include <algorithm>

// Hash a string (lowercased path) → 64-bit FNV-1a
static uint64_t HashPath64(const std::string& sIn) {
    std::string s = sIn;
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return (unsigned char)std::tolower(c); });

    const uint64_t FNV_OFFSET = 1469598103934665603ull;
    const uint64_t FNV_PRIME = 1099511628211ull;
    uint64_t h = FNV_OFFSET;
    for (unsigned char c : s) {
        h ^= (uint64_t)c;
        h *= FNV_PRIME;
    }
    return h;
}

// Compute local-space AABB from vertices
static void ComputeLocalAABB(const std::vector<Vertex>& vertices,
    glm::vec3& outMin, glm::vec3& outMax)
{
    if (vertices.empty()) { outMin = glm::vec3(0); outMax = glm::vec3(0); return; }
    outMin = outMax = vertices[0].pos;
    for (const auto& v : vertices) {
        outMin = glm::min(outMin, v.pos);
        outMax = glm::max(outMax, v.pos);
    }
}

// Convert local half extents to world half extents given rotation
static glm::vec3 RotatedAABBHalfExtents(const glm::quat& q, const glm::vec3& localHalf) {
    glm::mat3 R = glm::mat3_cast(q);
    glm::vec3 ax = glm::abs(glm::vec3(R[0]));
    glm::vec3 ay = glm::abs(glm::vec3(R[1]));
    glm::vec3 az = glm::abs(glm::vec3(R[2]));
    return glm::vec3(
        ax.x * localHalf.x + ay.x * localHalf.y + az.x * localHalf.z,
        ax.y * localHalf.x + ay.y * localHalf.y + az.y * localHalf.z,
        ax.z * localHalf.x + ay.z * localHalf.y + az.z * localHalf.z
    );
}

void ModelMeshComponent::onTransformUpdated(const glm::vec3& pos, const glm::vec3& scale, const glm::vec3& rot)
{
    for (auto* bo : m_bases) {
        if (!bo) continue;
        bo->setPosition(pos, scale, rot);
        SceneModelManager::getInstance().updateObjectTransform(bo, pos, scale, rot);
    }
}

void ModelMeshComponent::addModelToScene(const std::string& modelFile) {
    auto parent = getParent();
    if (!parent) return;

    // CPU load
    glm::vec3 color{ 1.f, 0.f, 1.f };
    std::vector<Vertex> vertices;
    std::vector< uint32_t> indices;
    ObjUtils::ParseOBJ(modelFile.c_str(), vertices, indices, false, false,false,true);  // we'll fix UVs below

    auto& sceneManager = SceneModelManager::getInstance();
    auto bo = sceneManager.addMeshModel(
        vertices, indices,
        parent->getTransform()->position,
        parent->getTransform()->scale,
        parent->getTransform()->rotation,
        m_material   // <<< pass the material so textures bind like primitives
    );

    m_bases.push_back(bo); // store the BaseObject

    if (m_groupByFile && bo) {
        bo->setLogicalGroupId(HashPath64(modelFile));
    }

    // whole-object coverage override (unchanged)
    if (bo) {
        const glm::vec3 pos = parent->getTransform()->position;
        const glm::vec3 scale = parent->getTransform()->scale;
        const glm::vec3 rdeg = parent->getTransform()->rotation;
        const glm::quat q = glm::quat(glm::radians(rdeg));

        glm::vec3 vmin, vmax;
        ComputeLocalAABB(vertices, vmin, vmax);
        glm::vec3 localHalf = 0.5f * (vmax - vmin);
        glm::vec3 scaledHalf = glm::abs(localHalf * scale);
        glm::vec3 worldHalf = RotatedAABBHalfExtents(q, scaledHalf);
        sceneManager.getMeshScene()->setObjectCoverageOverride(bo, pos, worldHalf);
    }

    if (m_makeGlobal && bo) {
        sceneManager.getMeshScene()->setObjectGlobal(bo, true);
    }
}