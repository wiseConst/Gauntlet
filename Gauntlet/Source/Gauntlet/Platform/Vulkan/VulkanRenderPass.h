#pragma once

#include "Gauntlet/Core/Core.h"
#include <volk/volk.h>

namespace Gauntlet
{
// TODO: Fill this while gonna implementin rendergraph

struct RenderPassSpecification
{
  public:
    RenderPassSpecification()  = default;
    ~RenderPassSpecification() = default;
};

class VulkanRenderPass final : private Uncopyable, private Unmovable
{
  public:
    VulkanRenderPass(const RenderPassSpecification& renderPassSpecification);
    VulkanRenderPass() = delete;
    ~VulkanRenderPass();

    void Begin(const VkCommandBuffer& commandBuffer, const VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE);
    FORCEINLINE void End(const VkCommandBuffer& commandBuffer) { vkCmdEndRenderPass(commandBuffer); }

    void Invalidate();
    void Destroy();

    FORCEINLINE auto& GetSpecification() const { return m_Specification; }
    FORCEINLINE auto& Get() const { return m_Handle; }

  private:
    VkRenderPass m_Handle = VK_NULL_HANDLE;
    RenderPassSpecification m_Specification;
};

}  // namespace Gauntlet