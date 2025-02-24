#include "PlaneColliderComponent.h"
#include "GameObject.h"

PlaneColliderComponent::PlaneColliderComponent(const glm::vec3& normal, float offset)
    : m_normal(normal), m_offset(offset)
{
    // For a static plane, the physics body is static (mass 0).
    m_body.position = glm::vec3(0.f);  // Not used for collision tests.
    m_body.velocity = glm::vec3(0.f);
    m_body.acceleration = glm::vec3(0.f);
    m_body.mass = 0.f;

    // Create a PlaneCollider using our custom physics code.
    m_collider = new PlaneCollider(ID, &m_body, normal, offset);
}

void PlaneColliderComponent::update()
{
    if (!m_parent)
        return;

    m_body.position = m_parent->getTransform()->position;
}
