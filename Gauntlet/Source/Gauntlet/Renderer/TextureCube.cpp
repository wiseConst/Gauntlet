#include "GauntletPCH.h"
#include "TextureCube.h"

#include "RendererAPI.h"
#include "Gauntlet/Platform/Vulkan/VulkanTextureCube.h"

namespace Gauntlet
{
Ref<TextureCube> TextureCube::Create(const std::vector<std::string>& faces)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            return Ref<VulkanTextureCube>(new VulkanTextureCube(faces));
        }
        case RendererAPI::EAPI::None: GNT_ASSERT(false, "RendererAPI is None!"); return nullptr;
    }

    GNT_ASSERT(false, "Unknown Renderer API!");
    return nullptr;
}

Ref<TextureCube> TextureCube::Create(const TextureCubeSpecification& textureCubeSpec)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            return Ref<VulkanTextureCube>(new VulkanTextureCube(textureCubeSpec));
        }
        case RendererAPI::EAPI::None: GNT_ASSERT(false, "RendererAPI is None!"); return nullptr;
    }

    GNT_ASSERT(false, "Unknown Renderer API!");
    return nullptr;
}

}  // namespace Gauntlet