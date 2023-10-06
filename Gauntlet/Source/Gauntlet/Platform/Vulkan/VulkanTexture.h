#pragma once

#include "Gauntlet/Core/Core.h"
#include "Gauntlet/Renderer/Texture.h"
#include "VulkanImage.h"

namespace Gauntlet
{
struct TextureCreateInfo
{
    const void* Data  = nullptr;
    size_t DataSize   = 0;
    uint32_t Width    = 0;
    uint32_t Height   = 0;
    uint16_t Channels = 4;
};

class VulkanTexture2D final : public Texture2D
{
  public:
    VulkanTexture2D(const std::string_view& textureFilePath, const TextureSpecification& textureSpecification);
    VulkanTexture2D(const void* data, const size_t size, const uint32_t imageWidth, const uint32_t imageHeight,
                    const TextureSpecification& textureSpecification = TextureSpecification());
    VulkanTexture2D() = default;

    ~VulkanTexture2D() = default;

    FORCEINLINE const uint32_t GetWidth() const final override { return m_Image->GetSpecification().Width; }
    FORCEINLINE const uint32_t GetHeight() const final override { return m_Image->GetSpecification().Height; }

    FORCEINLINE void* GetTextureID() const final override { return m_Image->GetTextureID(); }

    FORCEINLINE TextureSpecification& GetSpecification() final override { return m_Specification; }

    FORCEINLINE const auto& GetImage() const { return m_Image; }
    FORCEINLINE const auto& GetImage() { return m_Image; }

    FORCEINLINE const auto& GetImageDescriptorInfo() const { return m_Image->GetDescriptorInfo(); }
    FORCEINLINE auto& GetImageDescriptorInfo() { return m_Image->GetDescriptorInfo(); }

    void Destroy() final override;

  private:
    Ref<VulkanImage> m_Image;
    TextureSpecification m_Specification;

    void Create(const TextureCreateInfo& textureCreateInfo);
};

}  // namespace Gauntlet