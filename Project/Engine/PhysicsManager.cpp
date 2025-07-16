#include "PhysicsManager.h"
#include <stdexcept>

void PhysicsManager::Initialize() {
        auto& physx = PhysxBase::GetInstance();
        physx.initPhysics(false);
}

void PhysicsManager::StepPhysics(float deltaTime) {
    auto& physx = PhysxBase::GetInstance();
    physx.stepPhysics(false,deltaTime);
}

void PhysicsManager::Cleanup() {
    auto& physx = PhysxBase::GetInstance();
    physx.cleanupPhysics(false);
}
