#include "GauntletPCH.h"
#include "Texture.h"

#include "RendererAPI.h"
#include "Gauntlet/Renderer/Renderer.h"
#include "Gauntlet/Platform/Vulkan/VulkanTexture.h"

namespace Gauntlet
{

Ref<Texture2D> Texture2D::Create(const std::string_view& TextureFilePath)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            return MakeRef<VulkanTexture2D>(TextureFilePath);
        }
        case RendererAPI::EAPI::None:
        {
            GNT_ASSERT(false, "Renderer API is none!");
            break;
        }
    }

    GNT_ASSERT(false, "Unknown RHI!");
    return nullptr;
}
}  // namespace Gauntlet