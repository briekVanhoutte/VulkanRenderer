#pragma once
#include "../../vulkanbase/VulkanUtil.h"
enum class ColliderType { BOX, PLANE, CAPSULE };


struct PhysicsBody {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    float mass; // mass == 0 means static object.
};

class Collider {
public:
    Collider(int id, ColliderType type, PhysicsBody* body)
        : id(id), type(type), body(body) {
    }
    virtual ~Collider() {}

    int id;
    ColliderType type;
    PhysicsBody* body; // The collider uses the body's position

    // (We do not include a virtual collision function here.
    // Instead, our CollisionDetector class will dispatch based on type.)
};

class BoxCollider : public Collider {
public:
    BoxCollider(int id, PhysicsBody* body, const glm::vec3& halfExtents)
        : Collider(id, ColliderType::BOX, body), halfExtents(halfExtents) {
    }

    glm::vec3 halfExtents; // Half the width, height, and depth of the box
};

class PlaneCollider : public Collider {
public:
    // The plane is defined by a normal and an offset (i.e. plane equation: dot(normal, X) + offset = 0)
    PlaneCollider(int id, PhysicsBody* body, const glm::vec3& normal, float offset)
        : Collider(id, ColliderType::PLANE, body), normal(normal), offset(offset) {
    }

    glm::vec3 normal;
    float offset;
};

class CapsuleCollider : public Collider {
public:
    // The capsule is defined by a radius and a height (the height excluding the spherical ends)
    CapsuleCollider(int id, PhysicsBody* body, float radius, float height)
        : Collider(id, ColliderType::CAPSULE, body), radius(radius), height(height) {
    }

    float radius;
    float height;
};