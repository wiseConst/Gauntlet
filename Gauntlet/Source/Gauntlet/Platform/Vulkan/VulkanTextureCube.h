#pragma once

#include "Gauntlet/Renderer/TextureCube.h"
#include "VulkanImage.h"

namespace Gauntlet
{

class VulkanTextureCube final : public TextureCube
{
  public:
    VulkanTextureCube() = delete;
    VulkanTextureCube(const std::vector<std::string>& faces);
    VulkanTextureCube(const TextureCubeSpecification& textureCubeSpec);
    ~VulkanTextureCube() = default;

    void Destroy() final override;

    FORCEINLINE const auto& GetImageDescriptorInfo() const { return m_Image->GetDescriptorInfo(); }
    FORCEINLINE auto& GetImageDescriptorInfo() { return m_Image->GetDescriptorInfo(); }

  private:
    const std::vector<std::string> m_Faces;
    Ref<VulkanImage> m_Image;

    void Create();
};
}  // namespace Gauntlet