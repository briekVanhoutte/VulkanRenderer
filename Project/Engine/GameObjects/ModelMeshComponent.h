#pragma once

#include "Component.h"
#include <vector>
#include <string>
#include <iostream>
#include "../../vulkanbase/VulkanUtil.h" 

class ModelMeshComponent : public Component {
public:
    // Constructor takes the parent GameObject and the model file path.
    ModelMeshComponent(GameObject* parent, const std::string& modelFile) {
        setParent(parent);
        addModelToScene(modelFile);
    }

    void initialize() override {
       
    }
    void update() override {
       
    }
    void render() override {
        
    }

private:
    void addModelToScene(const std::string& modelFile); 
};
