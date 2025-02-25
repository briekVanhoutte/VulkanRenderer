#pragma once
#include "Physics/PhysxBase.h"

class PhysicsManager {
public:
    // Returns the single instance of PhysicsManager.
    static PhysicsManager& getInstance() {
        static PhysicsManager instance;
        return instance;
    }

    // Delete the copy constructor and assignment operator.
    PhysicsManager(const PhysicsManager&) = delete;
    PhysicsManager& operator=(const PhysicsManager&) = delete;

    // Update the physics simulation using the current delta time.
    void update(float deltaTime) {
        // Step the PhysX simulation with the variable deltaTime.
        PhysxBase::GetInstance().stepPhysics(deltaTime);
    }
    void initialize() {
        // Step the PhysX simulation with the variable deltaTime.
        PhysxBase::GetInstance().init();
    }

private:
    // Private constructor.
    PhysicsManager() {}
};
