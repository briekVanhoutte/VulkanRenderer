#include "PlaneColliderComponent.h"
#include "GameObject.h"

PlaneColliderComponent::PlaneColliderComponent(const PxVec3& normal, PxReal d)
    : m_normal(normal), m_d(d)
{
    // Use the PhysxBase helper to create a plane collider.
    // Planes are usually static.
    m_actor = PhysxBase::GetInstance().addPlaneCollider(m_normal, m_d);
}

PlaneColliderComponent::~PlaneColliderComponent() {
    m_actor = nullptr;
}

void PlaneColliderComponent::initialize() {
    // Additional initialization if needed.
}

void PlaneColliderComponent::update() {
    if (m_actor && m_parent) {
        // Although static, you can sync the GameObject's transform if desired.
        PxTransform pose = m_actor->getGlobalPose();
        m_parent->getTransform()->position = glm::vec3(pose.p.x, pose.p.y, pose.p.z);
        // (Optional) update rotation.
    }
}

void PlaneColliderComponent::render() {
    // Optionally render debug visuals.
}
