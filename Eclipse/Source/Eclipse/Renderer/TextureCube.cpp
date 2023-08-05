#include "EclipsePCH.h"
#include "TextureCube.h"

#include "RendererAPI.h"
#include "Eclipse/Platform/Vulkan/VulkanTextureCube.h"

namespace Eclipse
{
Ref<TextureCube> TextureCube::Create(const std::vector<std::string>& InFaces)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan: return Ref<VulkanTextureCube>(new VulkanTextureCube(InFaces));
        case RendererAPI::EAPI::None: ELS_ASSERT(false, "RendererAPI is None!"); return nullptr;
    }

    ELS_ASSERT(false, "Unknown Renderer API!");
    return nullptr;
}

}  // namespace Eclipse