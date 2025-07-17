#pragma once

#include <memory>
#include <string>
#include <Engine/Graphics/Texture.h>
#include <atomic>

class Material {
public:
    Material(const std::string& textureFilename);

    std::shared_ptr<Texture> getTexture() const { return m_Texture; }
    uint32_t getMaterialID() const { return m_MaterialID; }

private:
    std::shared_ptr<Texture> m_Texture;

    uint32_t m_MaterialID;
    static std::atomic<uint32_t> s_NextID;
};