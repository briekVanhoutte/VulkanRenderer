#include "RigidBodyComponent.h"
#include "GameObject.h"

RigidBodyComponent::RigidBodyComponent(float density)
    : m_density(density)
{
    // Use the GameObject's transform if available.
    PxTransform transform(PxVec3(0, 0, 0));
    if (m_parent) {
        glm::vec3 pos = m_parent->getTransform()->position;
        transform.p = PxVec3(pos.x, pos.y, pos.z);
    }
    // Create a dynamic rigid body using PhysxBase helper.
    // This helper attaches the actor to the scene.
    m_actor = PhysxBase::GetInstance().addRigidBody(transform, true, m_density);
}

RigidBodyComponent::~RigidBodyComponent() {
    // Optionally: remove the actor from the scene before destruction.
    m_actor = nullptr;
}

void RigidBodyComponent::initialize() {
    // Additional initialization if needed.
}

void RigidBodyComponent::update() {
    if (m_actor && m_parent) {
        // Sync the GameObject's transform with the actor's global pose.
        PxTransform pose = m_actor->getGlobalPose();
        m_parent->getTransform()->position = glm::vec3(pose.p.x, pose.p.y, pose.p.z);
        // std::cout <<ID << " " << pose.p.z << std::endl;
        // (Optional) update rotation if your GameObject supports it.
    }
}

void RigidBodyComponent::render() {
    // Optionally render debug visuals.
}
