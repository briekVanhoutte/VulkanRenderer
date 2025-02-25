#pragma once
#include <PxPhysicsAPI.h>
#include "Engine/Singleton.h"

using namespace physx;

class PhysxBase : public Singleton<PhysxBase>
{
public:
    void init();

    void stepPhysics(float deltaTime);

    void cleanup();

    PxRigidActor* addBoxCollider(const PxVec3& position, const PxVec3& halfExtents,
        bool dynamic = true,
        const PxQuat& rotation = PxQuat(PxIdentity),
        PxReal density = 1.0f);

    PxRigidActor* addPlaneCollider(const PxVec3& normal, PxReal d);

    PxRigidDynamic* addRigidBody(const PxTransform& transform,
        bool dynamic = true,
        PxReal density = 1.0f);

    PxScene* getScene() { return mScene; }
    PxPhysics* getPhysics() { return mPhysics; }
    PxMaterial* getMaterial() { return mMaterial; }

    ~PhysxBase() { cleanup(); }

private:
    PxFoundation* mFoundation = nullptr;
    PxPhysics* mPhysics = nullptr;
    PxDefaultCpuDispatcher* mDispatcher = nullptr;
    PxScene* mScene = nullptr;
    PxMaterial* mMaterial = nullptr;
    PxPvd* mPvd = nullptr;
};
