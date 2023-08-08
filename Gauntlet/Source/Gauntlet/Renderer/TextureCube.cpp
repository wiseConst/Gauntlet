#include "GauntletPCH.h"
#include "TextureCube.h"

#include "RendererAPI.h"
#include "Gauntlet/Platform/Vulkan/VulkanTextureCube.h"

namespace Gauntlet
{
Ref<TextureCube> TextureCube::Create(const std::vector<std::string>& InFaces)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan: return Ref<VulkanTextureCube>(new VulkanTextureCube(InFaces));
        case RendererAPI::EAPI::None: GNT_ASSERT(false, "RendererAPI is None!"); return nullptr;
    }

    GNT_ASSERT(false, "Unknown Renderer API!");
    return nullptr;
}

}  // namespace Gauntlet