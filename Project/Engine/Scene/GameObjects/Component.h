#pragma once

class GameObject;

class Component {
public:
    virtual ~Component() = default;

    void setParent(GameObject* parent) { m_parent = parent; }

    GameObject* getParent() const { return m_parent; }

    virtual void initialize() {}
    virtual void update() {}
    virtual void render() {}

protected:
    GameObject* m_parent = nullptr;
};
