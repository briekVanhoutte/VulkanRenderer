//#pragma once
//#include "Component.h"
//#include <PxPhysicsAPI.h>
//#include <stdexcept>
//#include "../PhysxBase.h"
//
//using namespace physx;
//
//class RigidBodyComponent : public Component {
//public:
//    // Constructor takes an optional flag for gravity and an initial transform.
//    RigidBodyComponent(bool enableGravity = true, const PxTransform& initialTransform = PxTransform(PxVec3(0.f, 0.f, 0.f)))
//        : m_enableGravity(enableGravity)
//    {
//        auto& physx = PhysxBase::GetInstance();
//        if (!gPhysics)
//            throw std::runtime_error("Physics not initialized!");
//
//        // Create a dynamic actor.
//        m_body = gPhysics->createRigidDynamic(initialTransform);
//        if (!m_body)
//            throw std::runtime_error("Failed to create RigidDynamic!");
//
//        // Set gravity flag accordingly.
//        m_body->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !m_enableGravity);
//
//        // (Optional: Attach shapes here using m_body->attachShape(...);)
//
//        // Add the actor to the scene.
//        physx.getScene()->addActor(*m_body);
//    }
//
//    virtual ~RigidBodyComponent() {
//        if (m_body)
//            m_body->release();
//    }
//
//    virtual void initialize() override {
//        // Additional initialization if required.
//    }
//
//    virtual void update() override; 
//
//    virtual void render() override {
//        // Optionally render debug information.
//    }
//
//    // Allow toggling gravity at runtime.
//    void setGravityEnabled(bool enabled) {
//        m_enableGravity = enabled;
//        if (m_body)
//            m_body->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !m_enableGravity);
//    }
//
//    PxRigidDynamic* getBody() const { return m_body; }
//
//private:
//    bool m_enableGravity;
//    PxRigidDynamic* m_body = nullptr;
//};
