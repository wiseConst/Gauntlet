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

void CreateImage(AllocatedImage* InImage, const uint32_t InWidth, const uint32_t InHeight, VkImageUsageFlags InImageUsageFlags,
                 VkFormat InFormat, VkImageTiling InImageTiling = VK_IMAGE_TILING_OPTIMAL, const uint32_t InMipLevels = 1,
                 const uint32_t InArrayLayers = 1);

VkFormat ChooseSupportedImageFormat(const VkPhysicalDevice& InDevice, const std::vector<VkFormat>& AvailableFormats,
                                    VkImageTiling InImageTiling, VkFormatFeatureFlags InFormatFeatures);

void CreateImageView(const VkDevice& InDevice, const VkImage& InImage, VkImageView* InImageView, VkFormat InFormat,
                     VkImageAspectFlags InAspectFlags, VkImageViewType InImageViewType = VK_IMAGE_VIEW_TYPE_2D);

VkFormat GauntletImageFormatToVulkan(EImageFormat InImageFormat);

void TransitionImageLayout(VkImage& InImage, VkImageLayout InOldLayout, VkImageLayout InNewLayout, const bool InIsCubeMap = false);

void CopyBufferDataToImage(const VkBuffer& InSourceBuffer, VkImage& InDestinationImage, const VkExtent3D& InImageExtent,
                           const bool InIsCubeMap = false);

VkFilter GauntletTextureFilterToVulkan(ETextureFilter InTextureFilter);

VkSamplerAddressMode GauntletTextureWrapToVulkan(ETextureWrap InTextureWrap);

}  // namespace ImageUtils

class VulkanImage final : public Image
{
  public:
    VulkanImage(const ImageSpecification& InImageSecification);
    ~VulkanImage() = default;

    void Create();
    void Destroy() final override;

    FORCEINLINE const auto& Get() const { return m_Image.Image; }
    FORCEINLINE auto& Get() { return m_Image.Image; }

    FORCEINLINE const auto& GetView() const { return m_Image.ImageView; }
    FORCEINLINE auto& GetView() { return m_Image.ImageView; }

    FORCEINLINE const auto& GetSampler() const { return m_Sampler; }
    FORCEINLINE auto& GetSampler() { return m_Sampler; }

    FORCEINLINE const auto GetFormat() const { return ImageUtils::GauntletImageFormatToVulkan(m_Specification.Format); }
    FORCEINLINE auto GetFormat() { return ImageUtils::GauntletImageFormatToVulkan(m_Specification.Format); }

    FORCEINLINE const ImageSpecification& GetSpecification() { return m_Specification; }

    FORCEINLINE uint32_t GetWidth() const final override { return m_Specification.Width; }
    FORCEINLINE uint32_t GetHeight() const final override { return m_Specification.Height; }
    FORCEINLINE float GetAspectRatio() const final override
    {
        GNT_ASSERT(GetHeight() != 0, "Zero divison!");
        return static_cast<float>(GetWidth() / GetHeight());
    }

    FORCEINLINE void SetExtent(const VkExtent2D& InExtent)
    {
        m_Specification.Width  = InExtent.width;
        m_Specification.Height = InExtent.height;
    }

    FORCEINLINE const auto& GetDescriptorInfo() const { return m_DescriptorImageInfo; }

  private:
    ImageSpecification m_Specification;

    AllocatedImage m_Image;
    VkSampler m_Sampler = VK_NULL_HANDLE;
    VkDescriptorImageInfo m_DescriptorImageInfo;
    VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;

    void CreateSampler();
};

}  // namespace Gauntlet