#include "PhysxBase.h"
#include <iostream>

// Global allocator and error callback required by PhysX.
static PxDefaultAllocator gAllocator;
static PxDefaultErrorCallback gErrorCallback;

#define PVD_HOST "127.0.0.1"

using namespace physx;

void PhysxBase::init()
{
    // Create foundation.
    mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
    if (!mFoundation)
    {
        std::cerr << "PxCreateFoundation failed!" << std::endl;
        return;
    }

    // Create PVD (PhysX Visual Debugger) connection.
    mPvd = PxCreatePvd(*mFoundation);
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
    mPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

    // Create the physics object.
    mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, PxTolerancesScale(), true, mPvd);
    if (!mPhysics)
    {
        std::cerr << "PxCreatePhysics failed!" << std::endl;
        return;
    }

    // Create the CPU dispatcher.
    mDispatcher = PxDefaultCpuDispatcherCreate(2);

    // Set up the scene description.
    PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = mDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    // Create the scene.
    mScene = mPhysics->createScene(sceneDesc);
    if (!mScene)
    {
        std::cerr << "createScene failed!" << std::endl;
        return;
    }

    // Create a default material.
    mMaterial = mPhysics->createMaterial(0.5f, 0.5f, 0.6f);

    // (Optional) Add a default ground plane.
    PxRigidStatic* groundPlane = PxCreatePlane(*mPhysics, PxPlane(0, 1, 0, 0), *mMaterial);
    mScene->addActor(*groundPlane);
}

void PhysxBase::stepPhysics(float deltaTime)
{
    if (mScene)
    {
        mScene->simulate(deltaTime);
        mScene->fetchResults(true);
    }
}

void PhysxBase::cleanup()
{
    if (mScene) { mScene->release();       mScene = nullptr; }
    if (mDispatcher) { mDispatcher->release();  mDispatcher = nullptr; }
    if (mPhysics) { mPhysics->release();     mPhysics = nullptr; }
    if (mPvd)
    {
        PxPvdTransport* transport = mPvd->getTransport();
        mPvd->release(); mPvd = nullptr;
        if (transport) { transport->release(); }
    }
    if (mFoundation) { mFoundation->release();  mFoundation = nullptr; }
}

PxRigidActor* PhysxBase::addBoxCollider(const PxVec3& position, const PxVec3& halfExtents,
    bool dynamic,
    const PxQuat& rotation,
    PxReal density)
{
    // Create the transform.
    PxTransform transform(position, rotation);
    PxRigidActor* actor = nullptr;

    if (dynamic)
    {
        // Create a dynamic actor.
        PxRigidDynamic* dynActor = mPhysics->createRigidDynamic(transform);
        actor = dynActor;
        // Create box geometry.
        PxBoxGeometry box(halfExtents);
        PxShape* shape = mPhysics->createShape(box, *mMaterial);
        dynActor->attachShape(*shape);
        shape->release();
        // Update mass and inertia using the provided density.
        PxRigidBodyExt::updateMassAndInertia(*dynActor, density);
    }
    else
    {
        // Create a static actor.
        actor = mPhysics->createRigidStatic(transform);
        PxBoxGeometry box(halfExtents);
        PxShape* shape = mPhysics->createShape(box, *mMaterial);
        actor->attachShape(*shape);
        shape->release();
    }
    mScene->addActor(*actor);
    return actor;
}

PxRigidActor* PhysxBase::addPlaneCollider(const PxVec3& normal, PxReal d)
{
    // Define the plane.
    PxPlane plane(normal.x, normal.y, normal.z, d);
    // Plane colliders are typically static.
    PxRigidStatic* planeActor = PxCreatePlane(*mPhysics, plane, *mMaterial);
    mScene->addActor(*planeActor);
    return planeActor;
}

PxRigidDynamic* PhysxBase::addRigidBody(const PxTransform& transform, bool dynamic, PxReal density)
{
    PxRigidDynamic* actor = nullptr;
    if (dynamic)
    {
        actor = mPhysics->createRigidDynamic(transform);
        PxRigidBodyExt::updateMassAndInertia(*actor, density);
    }
    else
    {
        // Although static actors are not dynamic, we return a pointer for consistency.
        actor = (PxRigidDynamic*)mPhysics->createRigidStatic(transform);
    }
    mScene->addActor(*actor);
    return actor;
}
