#pragma once

#include "Eclipse/Core/Core.h"
#include "Eclipse/Renderer/Texture.h"

#include "VulkanAllocator.h"

namespace Eclipse
{

struct AllocatedImage
{
  public:
    AllocatedImage() : Image(VK_NULL_HANDLE), Allocation(VK_NULL_HANDLE) {}
    ~AllocatedImage() = default;

    VkImage Image;
    VmaAllocation Allocation;
};

struct ImageSpecification
{
  public:
    ImageSpecification()
        : Samples(VK_SAMPLE_COUNT_1_BIT), Tiling(VK_IMAGE_TILING_OPTIMAL), ImageType(VK_IMAGE_TYPE_2D),
          ImageViewType(VK_IMAGE_VIEW_TYPE_2D), MipLevels(1), ArrayLayers(1), Extent({UINT32_MAX, UINT32_MAX, 1}),
          Usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT), Format(VK_FORMAT_UNDEFINED), ImageViewFormat(VK_FORMAT_UNDEFINED),
          ImageViewAspectFlags(VK_IMAGE_ASPECT_NONE){};
    ~ImageSpecification() = default;

    VkExtent3D Extent;
    VkFormat Format;
    uint32_t ArrayLayers;
    uint32_t MipLevels;
    VkSampleCountFlagBits Samples;
    VkImageUsageFlags Usage;
    VkImageTiling Tiling;
    VkImageType ImageType;

    VkFormat ImageViewFormat;
    VkImageAspectFlags ImageViewAspectFlags;
    VkImageViewType ImageViewType;
};

class VulkanImage final
{
  public:
    VulkanImage() = delete;
    VulkanImage(const ImageSpecification& InImageSpecification);
    ~VulkanImage() = default;

    void Create();
    void Destroy();

    FORCEINLINE const auto& Get() const { return m_AllocatedImage.Image; }
    FORCEINLINE auto& Get() { return m_AllocatedImage.Image; }

    FORCEINLINE const auto& GetView() const { return m_ImageView; }
    FORCEINLINE auto& GetView() { return m_ImageView; }

    FORCEINLINE const auto& GetFormat() const { return m_ImageSpecification.Format; }
    FORCEINLINE auto& GetFormat() { return m_ImageSpecification.Format; }

    FORCEINLINE void SetExtent(const VkExtent3D& InExtent) { m_ImageSpecification.Extent = InExtent; }

    static VkFormat ChooseSupportedImageFormat(const VkPhysicalDevice& InDevice, const std::vector<VkFormat>& AvailableFormats,
                                               VkImageTiling InImageTiling, VkFormatFeatureFlags InFormatFeatures);

    static void CreateImageView(const VkDevice& InDevice, const VkImage& InImage, VkImageView* InImageView, VkFormat InFormat,
                                VkImageAspectFlags InAspectFlags, VkImageViewType InImageViewType = VK_IMAGE_VIEW_TYPE_2D);

  private:
    AllocatedImage m_AllocatedImage;
    VkImageView m_ImageView;
    ImageSpecification m_ImageSpecification;
};

class VulkanTexture final : public Texture
{
  public:
  private:
};

}  // namespace Eclipse