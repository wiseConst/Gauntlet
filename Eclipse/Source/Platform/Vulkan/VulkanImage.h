#pragma once

#include "Eclipse/Renderer/Image.h"

#include "VulkanCore.h"
#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

namespace Eclipse
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

VkFormat EclipseImageFormatToVulkan(EImageFormat InImageFormat);

}  // namespace ImageUtils

class VulkanImage final : public Image
{
  public:
    VulkanImage(const ImageSpecification& InImageSecification, const char* InFilePath = nullptr);
    ~VulkanImage() = default;

    void Create();
    void Destroy() final override;

    FORCEINLINE const auto& Get() const { return m_Image.Image; }
    FORCEINLINE auto& Get() { return m_Image.Image; }

    FORCEINLINE const auto& GetView() const { return m_Image.ImageView; }
    FORCEINLINE auto& GetView() { return m_Image.ImageView; }

    FORCEINLINE const auto GetFormat() const { return ImageUtils::EclipseImageFormatToVulkan(m_ImageSpecification.Format); }
    FORCEINLINE auto GetFormat() { return ImageUtils::EclipseImageFormatToVulkan(m_ImageSpecification.Format); }

    FORCEINLINE const ImageSpecification& GetSpecification() { return m_ImageSpecification; }

    FORCEINLINE uint32_t GetWidth() const final override { return m_ImageSpecification.Width; }
    FORCEINLINE uint32_t GetHeight() const final override { return m_ImageSpecification.Height; }
    FORCEINLINE float GetAspectRatio() const final override
    {
        ELS_ASSERT(GetHeight() != 0, "Zero divison!");
        return static_cast<float>(GetWidth() / GetHeight());
    }

    FORCEINLINE void SetExtent(const VkExtent2D& InExtent)
    {
        m_ImageSpecification.Width = InExtent.width;
        m_ImageSpecification.Height = InExtent.height;
    }

  private:
    ImageSpecification m_ImageSpecification;

    AllocatedImage m_Image;
    VkSampler m_Sampler = VK_NULL_HANDLE;
};

}  // namespace Eclipse