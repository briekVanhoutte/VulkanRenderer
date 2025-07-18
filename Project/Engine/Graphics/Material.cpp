#include "Material.h"

std::atomic<uint32_t> Material::s_NextID = -1; // -1 to start at 0 because linked to textures on the gpu.

Material::Material(const std::string& textureFilename)
    : m_Texture(std::make_shared<Texture>(textureFilename)), m_MaterialID(s_NextID++)
{
}