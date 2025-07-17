#include "Material.h"

std::atomic<uint32_t> Material::s_NextID = 0;

Material::Material(const std::string& textureFilename)
    : m_Texture(std::make_shared<Texture>(textureFilename)), m_MaterialID(s_NextID++)
{
}