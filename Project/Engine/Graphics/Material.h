#pragma once

#include <memory>
#include <string>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/TextureManager.h> // <-- Include this!
#include <atomic>

class Material {
public:
    Material(const std::string& albedoMapFileName = "",
        const std::string& normalMapFileName = "",
        const std::string& metalnessMapFileName = "",
        const std::string& roughnessMapFileName = "",
        const std::string& heightMapFileName = "");

    std::shared_ptr<Texture> getAlbedoMapTexture() const { return m_AlbedoMapTexture; }
    std::shared_ptr<Texture> getNormalMapTexture()  const { return m_NormalMapTexture; }
    std::shared_ptr<Texture> getMetalnessMapTexture()  const { return m_MetalnessMapTexture; }
    std::shared_ptr<Texture> getRoughnessMapTexture()  const { return m_RoughnessMapTexture; }
    std::shared_ptr<Texture> getHeightMapTexture()  const { return m_HeightMapTexture; }

    uint32_t getAlbedoMapID() const { return m_AlbedoMapTexture ? m_AlbedoMapTexture->getID() : UINT32_MAX; }
    uint32_t getNormalMapID() const { return m_NormalMapTexture ? m_NormalMapTexture->getID() : UINT32_MAX; }
    uint32_t getMetalnessMapID() const { return m_MetalnessMapTexture ? m_MetalnessMapTexture->getID() : UINT32_MAX; ; }
    uint32_t getRoughnessMapID() const { return m_RoughnessMapTexture ? m_RoughnessMapTexture->getID() : UINT32_MAX; ; }
    uint32_t getHeightMapID() const { return m_HeightMapTexture ? m_HeightMapTexture->getID() : UINT32_MAX; ; }

    std::vector<std::shared_ptr<Texture>> getAllTextures() const;

private:
    std::shared_ptr<Texture> m_AlbedoMapTexture;
    std::shared_ptr<Texture> m_NormalMapTexture;
    std::shared_ptr<Texture> m_MetalnessMapTexture;
    std::shared_ptr<Texture> m_RoughnessMapTexture;
    std::shared_ptr<Texture> m_HeightMapTexture;

};