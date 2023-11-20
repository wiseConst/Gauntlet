#pragma once

#include "Gauntlet/Renderer/Material.h"

#include <volk/volk.h>
#include "VulkanDescriptors.h"

namespace Gauntlet
{
class VulkanMaterial final : public Material
{
  public:
    VulkanMaterial();
    ~VulkanMaterial() = default;

    void Invalidate() final override;
    void Destroy() final override;

    void Update() final override;

    FORCEINLINE const void* GetDescriptorSet() const final override { return m_DescriptorSet.Handle; }
    FORCEINLINE void* GetDescriptorSet() final override { return m_DescriptorSet.Handle; }

  private:
    DescriptorSet m_DescriptorSet;
};
}  // namespace Gauntlet