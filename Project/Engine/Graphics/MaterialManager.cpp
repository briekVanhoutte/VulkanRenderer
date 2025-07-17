#include "MaterialManager.h"

// Constructor
MaterialManager::MaterialManager() {
    m_standardMaterial = std::make_shared<Material>("Resources/Textures/errorTexture.jpg");
}

// Lookup or create material by file path (cached)
std::shared_ptr<Material> MaterialManager::getOrCreateMaterial(const std::string& filepath) {
    auto it = m_materialCache.find(filepath);
    if (it != m_materialCache.end())
        return it->second;
    auto material = std::make_shared<Material>(filepath);
    m_materialCache[filepath] = material;
    return material;
}

// Add material to active list (prevents duplicates)
void MaterialManager::addActiveMaterial(const std::shared_ptr<Material>& material) {
    if (!material) return;
    if (std::find(m_activeMaterials.begin(), m_activeMaterials.end(), material) == m_activeMaterials.end())
        m_activeMaterials.push_back(material);
}

// Remove material from active list
void MaterialManager::removeActiveMaterial(const std::shared_ptr<Material>& material) {
    auto it = std::remove(m_activeMaterials.begin(), m_activeMaterials.end(), material);
    m_activeMaterials.erase(it, m_activeMaterials.end());
}

// Replace active material list (optionally clears first)
void MaterialManager::setActiveMaterials(const std::vector<std::shared_ptr<Material>>& mats) {
    m_activeMaterials = mats;
}

// Find material by unique ID (linear search)
std::shared_ptr<Material> MaterialManager::getMaterialByID(uint32_t materialID) const {
    for (const auto& mat : m_activeMaterials)
        if (mat && mat->getMaterialID() == materialID)
            return mat;
    return nullptr;
}

// Find by index in active list (bounds checked)
std::shared_ptr<Material> MaterialManager::getMaterialByIndex(size_t idx) const {
    if (idx < m_activeMaterials.size()) return m_activeMaterials[idx];
    return nullptr;
}

// Clear the active material list
void MaterialManager::clearActiveMaterials() {
    m_activeMaterials.clear();
}

// Return all cached materials
std::vector<std::shared_ptr<Material>> MaterialManager::getAllCachedMaterials() const {
    std::vector<std::shared_ptr<Material>> result;
    for (const auto& [key, val] : m_materialCache)
        result.push_back(val);
    return result;
}
