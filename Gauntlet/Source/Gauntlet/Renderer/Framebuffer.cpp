#include "GauntletPCH.h"
#include "Framebuffer.h"

#include "RendererAPI.h"
#include "Gauntlet/Platform/Vulkan/VulkanFramebuffer.h"

namespace Gauntlet
{

Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& InFramebufferSpecification)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            return Ref<VulkanFramebuffer>(new VulkanFramebuffer(InFramebufferSpecification));
        }
        case RendererAPI::EAPI::None:
        {
            return nullptr;
        }
    }

    GNT_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

}  // namespace Gauntlet