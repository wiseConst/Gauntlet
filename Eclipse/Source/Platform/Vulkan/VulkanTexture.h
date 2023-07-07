#pragma once

#include "Eclipse/Core/Core.h"
#include "Eclipse/Renderer/Texture.h"
#include "VulkanImage.h"

namespace Eclipse
{

class VulkanTexture2D final : public Texture2D
{
  public:
    VulkanTexture2D() = delete;
    VulkanTexture2D(const std::string_view& TextureFilePath);
    ~VulkanTexture2D() = default;

    FORCEINLINE const uint32_t GetWidth() const final override { return m_Image->GetSpecification().Width; }
    FORCEINLINE const uint32_t GetHeight() const final override { return m_Image->GetSpecification().Height; }

    void Destroy() final override;

  private:
    Ref<VulkanImage> m_Image;
};

}  // namespace Eclipse