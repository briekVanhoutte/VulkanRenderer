#include "TextureManager.h"

// Constructor
TextureManager::TextureManager() {
    m_standardTexture = std::shared_ptr<Texture>(new Texture(kErrorTexturePath));
    m_textureCache.insert({ kErrorTexturePath ,m_standardTexture });
}

// Lookup or create texture by file path (cached)
std::shared_ptr<Texture> TextureManager::getOrCreateTexture(const std::string& filepath) {
    auto it = m_textureCache.find(filepath);
    if (it != m_textureCache.end())
        return it->second;
    auto texture = std::shared_ptr<Texture>(new Texture(filepath));
    
    m_textureCache[filepath] = texture;
    return texture;
}

// Add texture to active list (prevents duplicates)
void TextureManager::addActiveTexture(const std::shared_ptr<Texture>& texture) {
    if (!texture) return;
    if (std::find(m_activeTextures.begin(), m_activeTextures.end(), texture) == m_activeTextures.end()) {
        m_activeTextures.push_back(texture);
        m_textureListDirty = true;
    }
}

// Remove texture from active list
void TextureManager::removeActiveTexture(const std::shared_ptr<Texture>& texture) {
    auto it = std::remove(m_activeTextures.begin(), m_activeTextures.end(), texture);
    if (it != m_activeTextures.end()) {
        m_activeTextures.erase(it, m_activeTextures.end());
        m_textureListDirty = true;
    }
}

// Replace active texture list
void TextureManager::setActiveTextures(const std::vector<std::shared_ptr<Texture>>& textures) {
    m_activeTextures = textures;
    m_textureListDirty = true;
}

// Find texture by unique ID
std::shared_ptr<Texture> TextureManager::getTextureByID(uint32_t textureID) const {
    for (const auto& tex : m_activeTextures)
        if (tex && tex->getID() == textureID)
            return tex;
    return nullptr;
}

// Find by index in active list (bounds checked)
std::shared_ptr<Texture> TextureManager::getTextureByIndex(size_t idx) const {
    if (idx < m_activeTextures.size()) return m_activeTextures[idx];
    return nullptr;
}

// Clear the active texture list
void TextureManager::clearActiveTextures() {
    m_activeTextures.clear();
    m_textureListDirty = true;
}

// Return all cached textures
std::vector<std::shared_ptr<Texture>> TextureManager::getAllCachedTextures() const {
    std::vector<std::shared_ptr<Texture>> result;
    for (const auto& [key, val] : m_textureCache)
        result.push_back(val);
    return result;
}
