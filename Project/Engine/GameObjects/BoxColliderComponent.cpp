#include "BoxColliderComponent.h"
#include "GameObject.h"


BoxColliderComponent::BoxColliderComponent(float width, float height, float depth, bool dynamic)
    : m_width(width), m_height(height), m_depth(depth), m_dynamic(dynamic)
{
    // Determine the initial transform.
    PxTransform transform(PxVec3(0.f, 0.f, 0.f));
    if (m_parent)
    {
        glm::vec3 pos = m_parent->getTransform()->position;
        transform.p = PxVec3(pos.x, pos.y, pos.z);
    }

    // Create a dynamic actor or static actor depending on m_dynamic.
    if (m_dynamic)
    {
        m_actor = PhysxBase::GetInstance().getPhysics()->createRigidDynamic(transform);
    }
    else
    {
        m_actor = PhysxBase::GetInstance().getPhysics()->createRigidStatic(transform);
    }

    // Create a box shape with half extents.
    PxBoxGeometry boxGeom(m_width * 0.5f, m_height * 0.5f, m_depth * 0.5f);
    PxShape* shape = PhysxBase::GetInstance().getPhysics()->createShape(boxGeom, *PhysxBase::GetInstance().getMaterial());
    m_actor->attachShape(*shape);
    shape->release();

    // Add the actor to the scene.
    PhysxBase::GetInstance().getScene()->addActor(*m_actor);
}

void BoxColliderComponent::update() {
    if (m_actor && m_parent)
    {
        // Sync the GameObject's transform from the actor's global pose.
        PxTransform pose = { m_parent->getTransform()->position .x,m_parent->getTransform()->position.y,m_parent->getTransform()->position.z};
        m_actor->setGlobalPose(pose);

        // (Optional) Update rotation if your GameObject supports it.
    }
}
