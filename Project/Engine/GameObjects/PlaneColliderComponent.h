#pragma once
#include "Component.h"
#include "../Physics/PhysxBase.h" // Access to PhysxBase singleton

class PlaneColliderComponent : public Component {
public:
    // Constructor takes a plane normal and a distance (d) from the origin.
    PlaneColliderComponent(const PxVec3& normal, PxReal d);
    virtual ~PlaneColliderComponent();

    virtual void initialize() override;
    virtual void update() override;
    virtual void render() override;

    // Accessor for the underlying PhysX actor.
    PxRigidActor* getActor() const { return m_actor; }

private:
    PxVec3 m_normal;
    PxReal m_d;
    // The plane collider is typically static.
    PxRigidActor* m_actor = nullptr;
};
