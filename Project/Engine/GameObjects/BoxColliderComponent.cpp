#include "BoxColliderComponent.h"
#include "GameObject.h"


BoxColliderComponent::BoxColliderComponent(float width, float height, float depth, bool dynamic)
    : m_width(width), m_height(height), m_depth(depth), m_dynamic(dynamic)
{
    // Use parent's transform if available (or default to zero)
    if (m_parent)
        m_body.position = m_parent->getTransform()->position;
    else
        m_body.position = glm::vec3(0.f);

    m_body.velocity = glm::vec3(0.f);
    m_body.acceleration = glm::vec3(0.f);
    // For a dynamic object, assign a mass; otherwise static mass is 0.
    m_body.mass = (dynamic ? 1.f : 0.f);

    // Create a BoxCollider with half extents (width/2, height/2, depth/2)
    m_collider = new BoxCollider(ID, &m_body,
        glm::vec3(width * 0.5f, height * 0.5f, depth * 0.5f));
}

void BoxColliderComponent::update() {
    if (!m_parent)
        return;

    // Sync the GameObject's transform with the physics body's updated position.
    m_body.position = m_parent->getTransform()->position;
}
