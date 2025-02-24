#pragma once
#include "Component.h"   // Contains: struct PhysicsBody { glm::vec3 position, velocity, acceleration; float mass; };
#include "../Physics/Collider.h"  
#include <stdexcept>

// A plane collider is defined by a normal and an offset (i.e. plane equation: dot(normal, X) + offset = 0).
class PlaneColliderComponent : public Component {
public:
    // Constructor takes a normal vector and an offset.
    PlaneColliderComponent(const glm::vec3& normal, float offset);

    virtual ~PlaneColliderComponent() {
        delete m_collider;
    }

    virtual void initialize() override {
        // Nothing additional to initialize.
    }

    virtual void update() override;

    virtual void render() override {
        // Optionally render debug info.
    }

    // Accessor for the custom collider.
    Collider* getCollider() const { return m_collider; }
    PhysicsBody* getPhysicsBody() { return &m_body; }

private:
    glm::vec3 m_normal;
    float m_offset;
    PhysicsBody m_body;
    PlaneCollider* m_collider = nullptr;
};
