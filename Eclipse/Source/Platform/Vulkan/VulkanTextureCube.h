#pragma once

#include "Eclipse/Renderer/TextureCube.h"

namespace Eclipse
{
class VulkanTexture2D;
class VulkanImage;

class VulkanTextureCube final : public TextureCube
{
  public:
    VulkanTextureCube() = delete;
    VulkanTextureCube(const std::vector<std::string>& InFaces);
    ~VulkanTextureCube() = default;

    void Destroy() final override;

    FORCEINLINE const auto& GetImage() const { return m_Image; }

  private:
    const std::vector<std::string> m_Faces;
    Ref<VulkanImage> m_Image;

    void Create();
};
}  // namespace Eclipse