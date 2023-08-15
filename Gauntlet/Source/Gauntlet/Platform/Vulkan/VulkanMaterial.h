#pragma once

#include "Gauntlet/Renderer/Material.h"

#include <volk/volk.h>

namespace Gauntlet
{
class VulkanMaterial final : public Material
{
  public:
    VulkanMaterial();
    ~VulkanMaterial() = default;

    void Invalidate() final override;
    void Destroy();

    FORCEINLINE const auto& GetDescriptorSet() const { return m_DescriptorSet; }
    FORCEINLINE const bool IsMaterialLoaded() const { return m_bIsLoaded; }

  private:
    bool m_bIsLoaded{false};
    VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
};
}  // namespace Gauntlet