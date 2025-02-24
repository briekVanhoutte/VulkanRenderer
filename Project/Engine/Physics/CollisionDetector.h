// CollisionDetector.h
#pragma once
#include "Collider.h"
#include <vector>
#include <cmath>

class CollisionDetector {
public:
    // Given all colliders, return an n x n matrix (n = colliders.size()) of collision booleans.
    std::vector<std::vector<bool>> detectCollisions(const std::vector<Collider*>& colliders) {
        size_t n = colliders.size();
        std::vector<std::vector<bool>> matrix(n, std::vector<bool>(n, false));
        for (size_t i = 0; i < n; i++) {
            for (size_t j = i + 1; j < n; j++) {
                bool colliding = checkCollision(colliders[i], colliders[j]);
                matrix[i][j] = colliding;
                matrix[j][i] = colliding;
            }
        }
        return matrix;
    }

private:
    bool checkCollision(const Collider* a, const Collider* b) const {
        // Dispatch based on the collider types.
        if (a->type == ColliderType::BOX && b->type == ColliderType::BOX) {
            return checkBoxBox(static_cast<const BoxCollider*>(a), static_cast<const BoxCollider*>(b));
        }
        if (a->type == ColliderType::BOX && b->type == ColliderType::PLANE) {
            return checkBoxPlane(static_cast<const BoxCollider*>(a), static_cast<const PlaneCollider*>(b));
        }
        if (a->type == ColliderType::PLANE && b->type == ColliderType::BOX) {
            return checkBoxPlane(static_cast<const BoxCollider*>(b), static_cast<const PlaneCollider*>(a));
        }
        if (a->type == ColliderType::CAPSULE && b->type == ColliderType::CAPSULE) {
            return checkCapsuleCapsule(static_cast<const CapsuleCollider*>(a), static_cast<const CapsuleCollider*>(b));
        }
        if (a->type == ColliderType::BOX && b->type == ColliderType::CAPSULE) {
            return checkBoxCapsule(static_cast<const BoxCollider*>(a), static_cast<const CapsuleCollider*>(b));
        }
        if (a->type == ColliderType::CAPSULE && b->type == ColliderType::BOX) {
            return checkBoxCapsule(static_cast<const BoxCollider*>(b), static_cast<const CapsuleCollider*>(a));
        }
        if (a->type == ColliderType::PLANE && b->type == ColliderType::CAPSULE) {
            return checkPlaneCapsule(static_cast<const PlaneCollider*>(a), static_cast<const CapsuleCollider*>(b));
        }
        if (a->type == ColliderType::CAPSULE && b->type == ColliderType::PLANE) {
            return checkPlaneCapsule(static_cast<const PlaneCollider*>(b), static_cast<const CapsuleCollider*>(a));
        }
        // Default: if no matching function exists, assume no collision.
        return false;
    }

    // --- Collision function stubs ---

    // Simple AABB check for axis-aligned boxes
    bool checkBoxBox(const BoxCollider* a, const BoxCollider* b) const {
        glm::vec3 posA = a->body->position;
        glm::vec3 posB = b->body->position;
        glm::vec3 diff = glm::abs(posA - posB);
        glm::vec3 sum = a->halfExtents + b->halfExtents;
        return (diff.x <= sum.x && diff.y <= sum.y && diff.z <= sum.z);
    }

    // Check collision between an AABB and a plane.
    bool checkBoxPlane(const BoxCollider* box, const PlaneCollider* plane) const {
        float dist = glm::dot(plane->normal, box->body->position) + plane->offset;
        // Compute projection radius of the box onto the plane normal:
        float r = box->halfExtents.x * fabs(plane->normal.x) +
            box->halfExtents.y * fabs(plane->normal.y) +
            box->halfExtents.z * fabs(plane->normal.z);
        return fabs(dist) <= r;
    }

    // Simplified capsule-to-capsule: checks distance between centers.
    bool checkCapsuleCapsule(const CapsuleCollider* a, const CapsuleCollider* b) const {
        float dist = glm::length(a->body->position - b->body->position);
        return dist <= (a->radius + b->radius);
    }

    // Simplified box-to-capsule: treats capsule as a sphere at its center.
    bool checkBoxCapsule(const BoxCollider* box, const CapsuleCollider* capsule) const {
        glm::vec3 posBox = box->body->position;
        glm::vec3 posCapsule = capsule->body->position;
        glm::vec3 diff = glm::abs(posBox - posCapsule);
        glm::vec3 sum = box->halfExtents + glm::vec3(capsule->radius);
        return (diff.x <= sum.x && diff.y <= sum.y && diff.z <= sum.z);
    }

    // Check collision between a plane and a capsule (using capsule center)
    bool checkPlaneCapsule(const PlaneCollider* plane, const CapsuleCollider* capsule) const {
        float dist = glm::dot(plane->normal, capsule->body->position) + plane->offset;
        return fabs(dist) <= capsule->radius;
    }
};
