#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <Engine/Graphics/Texture.h>
#include <Engine/Core/Singleton.h>

class TextureManager : public Singleton<TextureManager> {
public:
    // Get or create texture by filepath (uses cache)
    std::shared_ptr<Texture> getOrCreateTexture(const std::string& filepath);

    // Standard texture getter/setter (e.g. a default white or error texture)
    void setStandardTexture(const std::shared_ptr<Texture>& texture) { m_standardTexture = texture; }
    std::shared_ptr<Texture> getStandardTexture() const { return m_standardTexture; }

    // Active textures getters/setters
    std::vector<std::shared_ptr<Texture>>& getActiveTextures() { return m_activeTextures; }
    const std::vector<std::shared_ptr<Texture>>& getActiveTextures() const { return m_activeTextures; }
    void setActiveTextures(const std::vector<std::shared_ptr<Texture>>& textures);

    // Add/remove texture to/from active list (prevents duplicates)
    void addActiveTexture(const std::shared_ptr<Texture>& texture);
    void removeActiveTexture(const std::shared_ptr<Texture>& texture);

    // Find by ID or index
    std::shared_ptr<Texture> getTextureByID(uint32_t textureID) const;
    std::shared_ptr<Texture> getTextureByIndex(size_t idx) const;

    // Get number of active textures
    size_t getNumActiveTextures() const { return m_activeTextures.size(); }

    // Clear active texture list
    void clearActiveTextures();

    // (Optional) Get all cached textures (not just active)
    std::vector<std::shared_ptr<Texture>> getAllCachedTextures() const;

    bool isTextureListDirty() const { return m_textureListDirty; }
    void clearTextureListDirty() { m_textureListDirty = false; }

private:
    friend class Singleton<TextureManager>;
    TextureManager();

    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textureCache;
    std::vector<std::shared_ptr<Texture>> m_activeTextures;
    std::shared_ptr<Texture> m_standardTexture;

    bool m_textureListDirty = true;
};
