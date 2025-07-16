#pragma once

#include "Component.h"
#include <vector>
#include <string>
#include <iostream>

class ModelMeshComponent : public Component {
public:
    ModelMeshComponent(GameObject* parent, const std::string& modelFile) {
        setParent(parent);
        addModelToScene(modelFile);
    }

    void initialize() override {
        std::cout << "ModelMeshComponent initialized." << std::endl;
    }
    void update() override {
        std::cout << "ModelMeshComponent updated." << std::endl;
    }
    void render() override {
        std::cout << "ModelMeshComponent rendered." << std::endl;
    }

private:
    void addModelToScene(const std::string& modelFile); 
};
