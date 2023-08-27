#pragma once

#include "Gauntlet/Renderer/Image.h"

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

namespace Gauntlet
{

struct AllocatedImage
{
  public:
    AllocatedImage() : Image(VK_NULL_HANDLE), ImageView(VK_NULL_HANDLE), Allocation(VK_NULL_HANDLE) {}
    ~AllocatedImage() = default;

    VkImage Image;
    VkImageView ImageView;
    VmaAllocation Allocation;
};

namespace ImageUtils
{

void CreateImage(AllocatedImage* image, const uint32_t width, const uint32_t height, VkImageUsageFlags imageUsageFlags, VkFormat format,
                 VkImageTiling imageTiling = VK_IMAGE_TILING_OPTIMAL, const uint32_t mipLevels = 1, const uint32_t arrayLayers = 1);

void CreateImageView(const VkDevice& device, const VkImage& image, VkImageView* imageView, VkFormat format, VkImageAspectFlags aspectFlags,
                     VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D);

VkFormat GauntletImageFormatToVulkan(EImageFormat imageFormat);

void TransitionImageLayout(VkImage& image, VkImageLayout oldLayout, VkImageLayout newLayout, const bool bIsCubeMap = false);

void CopyBufferDataToImage(const VkBuffer& sourceBuffer, VkImage& destinationImage, const VkExtent3D& imageExtent,
                           const bool bIsCubeMap = false);

VkFilter GauntletTextureFilterToVulkan(ETextureFilter textureFilter);

VkSamplerAddressMode GauntletTextureWrapToVulkan(ETextureWrap textureWrap);

}  // namespace ImageUtils

class VulkanImage final : public Image
{
  public:
    VulkanImage(const ImageSpecification& imageSecification);
    ~VulkanImage() = default;

    void Invalidate() final override;
    void Destroy() final override;

    FORCEINLINE auto& Get() { return m_Image.Image; }
    FORCEINLINE auto& GetView() { return m_Image.ImageView; }
    FORCEINLINE auto& GetSampler() { return m_Sampler; }
    FORCEINLINE auto GetFormat() { return ImageUtils::GauntletImageFormatToVulkan(m_Specification.Format); }
    FORCEINLINE ImageSpecification& GetSpecification() final override { return m_Specification; }

    FORCEINLINE uint32_t GetWidth() const final override { return m_Specification.Width; }
    FORCEINLINE uint32_t GetHeight() const final override { return m_Specification.Height; }
    FORCEINLINE float GetAspectRatio() const final override
    {
        GNT_ASSERT(GetHeight() != 0, "Zero divison!");
        return static_cast<float>(GetWidth() / GetHeight());
    }

    FORCEINLINE auto& GetDescriptorInfo() { return m_DescriptorImageInfo; }
    FORCEINLINE const auto& GetDescriptorInfo() const { return m_DescriptorImageInfo; }
    FORCEINLINE void* GetTextureID() const final override
    {
        if (!m_DescriptorSet) LOG_WARN("Attempting to return NULL image descriptor set!");
        return m_DescriptorSet;
    }

  private:
    ImageSpecification m_Specification;

    AllocatedImage m_Image;
    VkSampler m_Sampler = VK_NULL_HANDLE;
    VkDescriptorImageInfo m_DescriptorImageInfo;
    VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;

    void CreateSampler();
};

}  // namespace Gauntlet