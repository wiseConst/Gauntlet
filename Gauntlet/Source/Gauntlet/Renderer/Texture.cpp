#include "GauntletPCH.h"
#include "Texture.h"

#include "RendererAPI.h"
#include "Gauntlet/Renderer/Renderer.h"
#include "Gauntlet/Platform/Vulkan/VulkanTexture.h"

namespace Gauntlet
{

Ref<Texture2D> Texture2D::Create(const std::string_view& textureFilePath, const TextureSpecification& textureSpecification)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            return MakeRef<VulkanTexture2D>(textureFilePath, textureSpecification);
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

Ref<Texture2D> Texture2D::Create(const void* data, const size_t size, const uint32_t imageWidth, const uint32_t imageHeight,
                                 const TextureSpecification& textureSpecification)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            return MakeRef<VulkanTexture2D>(data, size, imageWidth, imageHeight, textureSpecification);
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