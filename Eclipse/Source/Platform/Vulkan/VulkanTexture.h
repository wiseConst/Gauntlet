#pragma once

#include "Eclipse/Core/Core.h"
#include "Eclipse/Renderer/Texture.h"
#include "VulkanImage.h"

namespace Eclipse
{

class VulkanTexture2D final : public Texture2D
{
  public:
    VulkanTexture2D(const std::string_view& TextureFilePath);
    VulkanTexture2D(const void* InData, const size_t InDataSize, const uint32_t InImageWidth, const uint32_t InImageHeight);
    VulkanTexture2D() = default;

    ~VulkanTexture2D() = default;

    FORCEINLINE const uint32_t GetWidth() const final override { return m_Image->GetSpecification().Width; }
    FORCEINLINE const uint32_t GetHeight() const final override { return m_Image->GetSpecification().Height; }

    FORCEINLINE const auto& GetImage() const { return m_Image; }
    FORCEINLINE const auto& GetImage() { return m_Image; }

    FORCEINLINE const auto& GetSampler() const { return m_Sampler; }
    FORCEINLINE auto& GetSampler() { return m_Sampler; }

    void Destroy() final override;

  private:
    Ref<VulkanImage> m_Image;

    VkSampler m_Sampler = VK_NULL_HANDLE;
};

}  // namespace Eclipse