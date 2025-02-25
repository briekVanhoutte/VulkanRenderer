#pragma once
#include "Component.h"
#include "../Physics/PhysxBase.h" // Ensure you can access the singleton.

class BoxColliderComponent : public Component {
public:
    // Constructor: width, height, depth are the full dimensions.
    // 'dynamic' indicates whether the object is dynamic.
    BoxColliderComponent(float width, float height, float depth, bool dynamic = false);
       

    virtual ~BoxColliderComponent() {
        if (m_actor)
        {
            // Optionally remove the actor from the scene before releasing.
            m_actor = nullptr;
        }
    }

    virtual void initialize() override {
        // Additional initialization if needed.
    }

    virtual void update() override;

    virtual void render() override {
        // Optionally render debug visuals.
    }

    // Provide access to the underlying PhysX actor.
    PxRigidActor* getActor() const { return m_actor; }

private:
    float m_width, m_height, m_depth;
    bool m_dynamic;
    PxRigidActor* m_actor = nullptr;
};
