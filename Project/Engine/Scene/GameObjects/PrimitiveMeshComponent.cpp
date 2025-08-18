#include "PrimitiveMeshComponent.h"
#include "GameObject.h"


static inline std::pair<float, float> faceWHForNormal(const glm::vec3& n,
    float W, float H, float D,
    const glm::vec3& S)
{
    // Map base dims to face W/H, then apply parent scale components
    if (std::abs(n.z) > 0.5f) { // ±Z face -> width=X, height=Y
        return { W * S.x, H * S.y };
    }
    else if (std::abs(n.x) > 0.5f) { // ±X face -> width=Z, height=Y
        return { D * S.z, H * S.y };
    }
    else { // ±Y face -> width=X, height=Z
        return { W * S.x, D * S.z };
    }
}

static void BuildCubeGeometry(float w, float h, float d,
    std::vector<Vertex>& outV,
    std::vector<uint32_t>& outI)
{
    outV.clear(); outI.clear();
    const float hx = w * 0.5f, hy = h * 0.5f, hz = d * 0.5f;

    auto pushFace = [&](glm::vec3 n,
        glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d,  // CCW as seen from outside
        glm::vec2 ta = { 0,0 }, glm::vec2 tb = { 1,0 }, glm::vec2 tc = { 1,1 }, glm::vec2 td = { 0,1 })
        {
            const uint32_t base = (uint32_t)outV.size();
            const glm::vec3 col(1.0f);
            outV.push_back(Vertex{ a, n, col, ta });
            outV.push_back(Vertex{ b, n, col, tb });
            outV.push_back(Vertex{ c, n, col, tc });
            outV.push_back(Vertex{ d, n, col, td });
            // fixed CCW indices
            outI.insert(outI.end(), { base + 0, base + 1, base + 2,  base + 0, base + 2, base + 3 });
        };

    // +Z (front)  : viewer in +Z looking toward -Z  -> CCW: BL, BR, TR, TL
    pushFace({ 0,0, 1 },
        { -hx,-hy, hz }, { +hx,-hy, hz }, { +hx,+hy, hz }, { -hx,+hy, hz });

    // -Z (back)   : viewer in -Z looking toward +Z  -> CCW: BL, BR, TR, TL (note different corners)
    pushFace({ 0,0,-1 },
        { +hx,-hy,-hz }, { -hx,-hy,-hz }, { -hx,+hy,-hz }, { +hx,+hy,-hz });

    // -X (left)   : viewer at -X looking toward +X
    pushFace({ -1,0,0 },
        { -hx,-hy,-hz }, { -hx,-hy, hz }, { -hx,+hy, hz }, { -hx,+hy,-hz });

    // +X (right)  : viewer at +X looking toward -X
    pushFace({ +1,0,0 },
        { +hx,-hy, hz }, { +hx,-hy,-hz }, { +hx,+hy,-hz }, { +hx,+hy, hz });

    // -Y (bottom) : viewer at -Y looking toward +Y
    pushFace({ 0,-1,0 },
        { -hx,-hy,-hz }, { +hx,-hy,-hz }, { +hx,-hy, hz }, { -hx,-hy, hz });

    // +Y (top)    : viewer at +Y looking toward -Y
    pushFace({ 0,+1,0 },
        { -hx,+hy, hz }, { +hx,+hy, hz }, { +hx,+hy,-hz }, { -hx,+hy,-hz });
}

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


void PrimitiveMeshComponent::onTransformUpdated(const glm::vec3& pos, const glm::vec3& scale, const glm::vec3& rot)
{
    // Planes (single base) still OK:
    if (m_faces.empty()) {
        for (auto* bo : m_bases) {
            if (!bo) continue;
            SceneModelManager::getInstance().updateObjectTransform(bo, pos, scale, rot);
        }
        return;
    }

    // Cubes: rebuild each face transform from parent transform + stored local data
    auto& sm = SceneModelManager::getInstance();
    MeshScene* ms = sm.getMeshScene();

    const glm::quat qParent = glm::quat(glm::radians(rot));
    const float W = m_cubeDims.x, H = m_cubeDims.y, D = m_cubeDims.z;

    // Recompute the cube's group AABB half-extents for coverage override:
    const glm::vec3 halfBase(W * 0.5f * scale.x, H * 0.5f * scale.y, D * 0.5f * scale.z);
    // abs(R) * halfBase
    const glm::mat3 R = glm::mat3_cast(qParent);
    const glm::vec3 ax = glm::abs(glm::vec3(R[0]));
    const glm::vec3 ay = glm::abs(glm::vec3(R[1]));
    const glm::vec3 az = glm::abs(glm::vec3(R[2]));
    const glm::vec3 he{
        ax.x * halfBase.x + ay.x * halfBase.y + az.x * halfBase.z,
        ax.y * halfBase.x + ay.y * halfBase.y + az.y * halfBase.z,
        ax.z * halfBase.x + ay.z * halfBase.y + az.z * halfBase.z
    };

    for (auto& f : m_faces) {
        if (!f.bo) continue;

        // (1) center: scale local center per axis, then rotate by parent, then translate
        const glm::vec3 centerOffScaled = f.localCenter * scale; // component-wise
        const glm::vec3 worldCenter = pos + qParent * centerOffScaled;

        // (2) size under current scale (depends on which face)
        const auto [wScaled, hScaled] = faceWHForNormal(f.localNormal, W, H, D, scale);
        const glm::vec3 faceScale(wScaled, hScaled, 1.0f);

        // (3) rotation: align +Z to the **world** face normal, then apply parent
        const glm::vec3 worldNormal = glm::normalize(qParent * f.localNormal);
        const glm::quat qAlign = glm::rotation(glm::vec3(0, 0, 1), worldNormal);
        const glm::quat qFinal = qParent * qAlign;   // same composition as at spawn
        const glm::vec3 rotDeg = glm::degrees(glm::eulerAngles(qFinal));

        // (4) push to renderable & chunk grid
        sm.updateObjectTransform(f.bo, worldCenter, faceScale, rotDeg);

        // keep the faces grouped for culling (optional but matches your original intent)
        ms->setObjectCoverageOverride(f.bo, pos, he);
    }
}

void PrimitiveMeshComponent::addPrimitiveToScene(PrimitiveType type, float width, float height, float depth,
    const std::shared_ptr<Material> mat)
{
    auto parent = getParent();
    if (!parent) return;

    auto& sceneManager = SceneModelManager::getInstance();

    if (type == PrimitiveType::Plane) {
        // unchanged
        glm::vec3 normal{ 0.f, 1.f, 0.f };
        auto bo = sceneManager.addMeshRectangle(normal, glm::vec3(1, 0, 1),
            width, height,
            parent->getTransform()->position,
            parent->getTransform()->scale,
            parent->getTransform()->rotation,
            mat);
        m_bases.push_back(bo);
        if (m_makeGlobal) sceneManager.getMeshScene()->setObjectGlobal(bo, true);
        return;
    }
    MeshScene* ms = sceneManager.getMeshScene();

    // Build geometry with the requested dimensions baked in.
    std::vector<Vertex> verts;
    std::vector<uint32_t> inds;
    BuildCubeGeometry(width, height, depth, verts, inds);

    // Use the parent's transform directly.
    const glm::vec3 pos = parent->getTransform()->position;
    const glm::vec3 rot = parent->getTransform()->rotation;   // degrees
    const glm::vec3 scale = parent->getTransform()->scale;

    // Create a single renderable object for the cube.
    BaseObject* bo = sceneManager.addMeshModel(
        verts, inds,
        pos, scale, rot,
        mat
    );
    if (bo) {
        m_bases.push_back(bo);

        // Give the chunk grid a proper world-space AABB so all faces survive culling.
        const glm::quat q = glm::quat(glm::radians(rot));
        const glm::vec3 localHalf(width * 0.5f, height * 0.5f, depth * 0.5f);
        const glm::vec3 scaledHalf = glm::abs(localHalf * scale);
        const glm::vec3 worldHalf = RotatedAABBHalfExtents(q, scaledHalf);
        ms->setObjectCoverageOverride(bo, pos, worldHalf);

        if (m_makeGlobal) ms->setObjectGlobal(bo, true);
    }
    return;
}