#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <Engine/Graphics/Material.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Core/Singleton.h>

class MaterialManager : public Singleton<MaterialManager> {
public:
    // Get or create material by filepath (uses cache)
    std::shared_ptr<Material> getOrCreateMaterial(const std::string& filepath);

    // Standard material getter/setter
    void setStandardMaterial(const std::shared_ptr<Material>& material) { m_standardMaterial = material; }
    std::shared_ptr<Material> getStandardMaterial() const { return m_standardMaterial; }

    // Active materials getters/setters
    std::vector<std::shared_ptr<Material>>& getActiveMaterials() { return m_activeMaterials; }
    const std::vector<std::shared_ptr<Material>>& getActiveMaterials() const { return m_activeMaterials; }
    void setActiveMaterials(const std::vector<std::shared_ptr<Material>>& mats);

    // Add/remove material to/from active list (prevents duplicates)
    void addActiveMaterial(const std::shared_ptr<Material>& material);
    void removeActiveMaterial(const std::shared_ptr<Material>& material);

    // Find by ID or index (utility)
    std::shared_ptr<Material> getMaterialByID(uint32_t materialID) const;
    std::shared_ptr<Material> getMaterialByIndex(size_t idx) const;

    // Get number of active materials/textures
    size_t getNumActiveMaterials() const { return m_activeMaterials.size(); }

    // Clear active material list
    void clearActiveMaterials();

    // (Optional) Get all cached materials (not just active)
    std::vector<std::shared_ptr<Material>> getAllCachedMaterials() const;
private:
    friend class Singleton<MaterialManager>;
    MaterialManager();

    std::unordered_map<std::string, std::shared_ptr<Material>> m_materialCache;
    std::vector<std::shared_ptr<Material>> m_activeMaterials;
    std::shared_ptr<Material> m_standardMaterial;
};
