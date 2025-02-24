#pragma once
#include "Physics/PhysicsBody.h"
#include "Physics/Collider.h"
#include "Physics/CollisionDetector.h"
#include <vector>
#include <memory>

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

    // Add a new physics body and its collider to the simulation.
    void addBody(PhysicsBody* body, Collider* collider) {
        bodies.push_back(body);
        colliders.push_back(collider);
    }

    // Update the physics simulation.
    void update(float deltaTime) {
        // Simple Euler integration:
        for (auto body : bodies) {
            if (body->mass > 0.0f) { // dynamic object
                body->acceleration = glm::vec3(0.0f, -9.81f, 0.0f); // gravity
                body->velocity += body->acceleration * deltaTime;
                body->position += body->velocity * deltaTime;
            }
        }
        // Run collision detection to update the collision matrix.
        collisionMatrix = collisionDetector.detectCollisions(colliders);
    }

    // Query if two colliders (by their IDs) are colliding.
    bool areColliding(int colliderID1, int colliderID2) const {
        int index1 = getColliderIndexByID(colliderID1);
        int index2 = getColliderIndexByID(colliderID2);
        if (index1 == -1 || index2 == -1) return false;
        return collisionMatrix[index1][index2];
    }

private:
    // Private constructor to prevent instantiation.
    PhysicsManager() {}

    std::vector<PhysicsBody*> bodies;
    std::vector<Collider*> colliders;
    CollisionDetector collisionDetector;
    std::vector<std::vector<bool>> collisionMatrix;

    // Helper to find a collider by its ID.
    int getColliderIndexByID(int id) const {
        for (size_t i = 0; i < colliders.size(); i++) {
            if (colliders[i]->id == id)
                return static_cast<int>(i);
        }
        return -1;
    }
};
