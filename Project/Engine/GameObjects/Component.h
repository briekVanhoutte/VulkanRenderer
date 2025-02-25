#pragma once

// Forward declaration to avoid circular dependency.
class GameObject;

class Component {
public:
    virtual ~Component() = default;

    // Called to set the parent GameObject.
    void setParent(GameObject* parent) { m_parent = parent;}

    // Retrieve the parent GameObject.
    GameObject* getParent() const { return m_parent; }

    // Lifecycle methods.
    virtual void initialize() { ID = 0; }
    virtual void update() {}
    virtual void render() {}

protected:
    GameObject* m_parent = nullptr; // Link to the parent GameObject.
    inline static int ID = 0;
};


