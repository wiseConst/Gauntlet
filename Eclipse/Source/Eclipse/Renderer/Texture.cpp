#include "EclipsePCH.h"
#include "Texture.h"

#include "RendererAPI.h"
#include "Platform/Vulkan/VulkanTexture.h"

namespace Eclipse
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
            ELS_ASSERT(false, "Renderer API is none!");
            break;
        }
    }

    ELS_ASSERT(false, "Unknown RHI!");
    return nullptr;
}
}  // namespace Eclipse