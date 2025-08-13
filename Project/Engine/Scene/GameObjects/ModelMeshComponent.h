#pragma once

#include "Component.h"
#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <Engine/Graphics/Material.h>

class ModelMeshComponent : public Component {
public:
    // NEW: take an optional material (default null like your primitives)
    ModelMeshComponent(GameObject* parent,
        const std::string& modelFile,
        const std::shared_ptr<Material> mat = {},
        bool makeGlobal = false,
        bool groupByFile = true)
        : m_modelFile(modelFile)
        , m_material(mat)
        , m_makeGlobal(makeGlobal)
        , m_groupByFile(groupByFile)
    {
        setParent(parent);
        addModelToScene(modelFile);
    }

    void initialize() override { }
    void update() override {  }
    void render() override {  }

private:
    void addModelToScene(const std::string& modelFile);

    std::string m_modelFile;
    std::shared_ptr<Material> m_material;   // NEW
    bool m_makeGlobal = false;
    bool m_groupByFile = true;
};
