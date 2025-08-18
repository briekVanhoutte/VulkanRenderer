#pragma once
#include <glm/glm.hpp>

class GameObject;

class Component {
public:
    virtual ~Component() = default;

    void setParent(GameObject* parent) { m_parent = parent; }

    GameObject* getParent()const  { return m_parent; }

    virtual void initialize() {}
    virtual void update() {}
    virtual void render() {}

    // NEW: by default, components ignore transform updates
    virtual void onTransformUpdated(const glm::vec3& pos,
        const glm::vec3& scale,
        const glm::vec3& rot) {


    }

protected:
    GameObject* m_parent = nullptr;
};
