#pragma once
#include <PxPhysicsAPI.h>
#include "PhysxBase.h"
using namespace physx;

class PhysicsManager {
public:
    static PhysicsManager& GetInstance() {
        static PhysicsManager instance;
        return instance;
    }

    void Initialize();

    void StepPhysics(float deltaTime);

    void Cleanup();

private:
    PhysicsManager() = default;
    ~PhysicsManager() = default;
    PhysicsManager(const PhysicsManager&) = delete;
    PhysicsManager& operator=(const PhysicsManager&) = delete;

};
