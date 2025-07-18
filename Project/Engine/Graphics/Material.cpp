#include "Material.h"

#include "Material.h"
#include <Engine/Graphics/TextureManager.h>
#include <Engine/Graphics/vulkanVars.h>
Material::Material(const std::string& albedoMapFileName,
    const std::string& normalMapFileName,
    const std::string& metalnessMapFileName,
    const std::string& roughnessMapFileName,
    const std::string& heightMapFileName)
{
    // Use the TextureManager to fetch or create textures.
    if (!albedoMapFileName.empty()) {
        m_AlbedoMapTexture = TextureManager::GetInstance().getOrCreateTexture(albedoMapFileName);
    }
    else {
        m_AlbedoMapTexture = TextureManager::GetInstance().getStandardTexture();
    }

    if (!normalMapFileName.empty()) {
        m_NormalMapTexture = TextureManager::GetInstance().getOrCreateTexture(normalMapFileName);
    }
    else {
        m_NormalMapTexture = nullptr;
    }

    if (!metalnessMapFileName.empty()) {
        m_MetalnessMapTexture = TextureManager::GetInstance().getOrCreateTexture(metalnessMapFileName);
    }
    else {
        m_MetalnessMapTexture = nullptr;
    }

    if (!roughnessMapFileName.empty()) {
        m_RoughnessMapTexture = TextureManager::GetInstance().getOrCreateTexture(roughnessMapFileName);
    }
    else {
        m_RoughnessMapTexture = nullptr;
    }
    if (!heightMapFileName.empty()) {
        m_HeightMapTexture = TextureManager::GetInstance().getOrCreateTexture(heightMapFileName);
    }
    else {
        m_HeightMapTexture = nullptr;
    }
}

std::vector<std::shared_ptr<Texture>> Material::getAllTextures() const {
    std::vector<std::shared_ptr<Texture>> textures;
    if (m_AlbedoMapTexture)    textures.push_back(m_AlbedoMapTexture);
    if (m_NormalMapTexture)    textures.push_back(m_NormalMapTexture);
    if (m_MetalnessMapTexture) textures.push_back(m_MetalnessMapTexture);
    if (m_RoughnessMapTexture) textures.push_back(m_RoughnessMapTexture);
    if (m_HeightMapTexture)    textures.push_back(m_HeightMapTexture);
    return textures;
}
