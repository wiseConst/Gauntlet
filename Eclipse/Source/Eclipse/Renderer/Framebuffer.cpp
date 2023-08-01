#include "EclipsePCH.h"
#include "Framebuffer.h"

#include "RendererAPI.h"
#include "Platform/Vulkan/VulkanFramebuffer.h"

namespace Eclipse
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

    ELS_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

}  // namespace Eclipse