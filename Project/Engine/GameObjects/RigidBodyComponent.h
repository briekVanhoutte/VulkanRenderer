#pragma once
#include "Component.h"
#include "../Physics/PhysxBase.h" // Access to PhysxBase singleton

class RigidBodyComponent : public Component {
public:
    // Constructor accepts an optional density (for mass/inertia calculations).
    RigidBodyComponent(float density = 1.0f);
    virtual ~RigidBodyComponent();

    virtual void initialize() override;
    virtual void update() override;
    virtual void render() override;

    // Accessor for the underlying PhysX dynamic actor.
    PxRigidDynamic* getActor() const { return m_actor; }

private:
    float m_density;
    // This component is intended to work with dynamic rigid bodies.
    PxRigidDynamic* m_actor = nullptr;
};
