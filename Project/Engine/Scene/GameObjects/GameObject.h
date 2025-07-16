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
        // Automatically add a TransformComponent to every GameObject.
        m_transform = addComponent<TransformComponent>();
    }

    ~GameObject() = default;

    // Template function to add any component type.
    template<typename T, typename... Args>
    T* addComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* rawPtr = component.get();
        // Link the component back to this GameObject.
        rawPtr->setParent(this);
        m_components.push_back(std::move(component));
        return rawPtr;
    }

    // Template function to check if a component of type T exists.
    template<typename T>
    bool hasComponent() const {
        for (const auto& comp : m_components) {
            if (dynamic_cast<T*>(comp.get()))
                return true;
        }
        return false;
    }

    // Lifecycle functions to propagate calls to all components.
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

    // Convenience accessor for the default TransformComponent.
    TransformComponent* getTransform() const { return m_transform; }

private:
    std::vector<std::unique_ptr<Component>> m_components;
    TransformComponent* m_transform; // Cached pointer to the GameObject's transform.
};
