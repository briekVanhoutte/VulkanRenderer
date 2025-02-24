#pragma once
#include "Component.h"  // Contains: struct PhysicsBody { glm::vec3 position, velocity, acceleration; float mass; };
#include "../Physics/Collider.h"      // Contains: class Collider, BoxCollider, PlaneCollider, CapsuleCollider, etc.

// This component now uses our custom physics system instead of PhysX.
class BoxColliderComponent : public Component {
public:
    // width, height, depth are the full dimensions; dynamic indicates whether the body is dynamic.
    BoxColliderComponent(float width, float height, float depth, bool dynamic = false);

    virtual ~BoxColliderComponent() {
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
    float m_width, m_height, m_depth;
    bool m_dynamic;
    PhysicsBody m_body;
    BoxCollider* m_collider = nullptr;
};
