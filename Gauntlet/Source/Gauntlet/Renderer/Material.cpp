#include "GauntletPCH.h"

#include "Gauntlet/Renderer/RendererAPI.h"
#include "Gauntlet/Platform/Vulkan/VulkanMaterial.h"

namespace Gauntlet
{
Ref<Material> Material::Create()
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            return MakeRef<VulkanMaterial>();
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