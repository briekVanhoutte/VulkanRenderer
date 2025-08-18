#pragma once

#include <vector>
#include <memory>
#include <type_traits>
#include "Component.h"
#include "TransformComponent.h"
#include "ModelMeshComponent.h"
#include "PrimitiveMeshComponent.h"

class GameObject {
public:
    GameObject() {
        m_transform = addComponent<TransformComponent>();
    }

    ~GameObject() = default;

    template<typename T, typename... Args>
    T* addComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* rawPtr = component.get();
        rawPtr->setParent(this);
        m_components.push_back(std::move(component));
        return rawPtr;
    }

    template<typename T>
    bool hasComponent() const {
        for (const auto& comp : m_components) {
            if (dynamic_cast<T*>(comp.get()))
                return true;
        }
        return false;
    }

    void initialize() {
        for (auto& comp : m_components)
            comp->initialize();
    }

    void update() {
        for (auto& comp : m_components)
            comp->update();
    }

    void render() {
        for (auto& comp : m_components)
            comp->render();
    }

    TransformComponent* getTransform() const { return m_transform; }

    void broadcastTransformChanged() {
        const auto* t = m_transform;
        for (auto& up : m_components) {
            Component* c = up.get();
            if (c == m_transform) continue; // no need to notify self
            c->onTransformUpdated(t->position, t->scale, t->rotation);
        }
    }

private:
    std::vector<std::unique_ptr<Component>> m_components;
    TransformComponent* m_transform; 
};
