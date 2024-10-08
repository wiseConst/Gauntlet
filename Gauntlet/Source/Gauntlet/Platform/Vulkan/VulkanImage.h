#pragma once

#include "Gauntlet/Renderer/Image.h"

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>
#include "VulkanDescriptors.h"

#pragma warning(disable : 4834)

namespace Gauntlet
{

struct AllocatedImage final
{
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
                     VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D, const uint32_t mipLevels = 1);

VkFormat GauntletImageFormatToVulkan(EImageFormat imageFormat);

void TransitionImageLayout(VkImage& image, VkImageLayout oldLayout, VkImageLayout newLayout, EImageFormat format,
                           const uint32_t mipLevels = 1, const bool bIsCubeMap = false);

void CopyBufferDataToImage(const VkBuffer& sourceBuffer, VkImage& destinationImage, const VkExtent3D& imageExtent,
                           const bool bIsCubeMap = false);

void GenerateMipmaps(VkImage& image, const VkFormat format, const VkFilter filter, const uint32_t width, const uint32_t height,
                     const uint32_t mipLevels);

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

    FORCEINLINE auto GetLayout() const { return m_Layout; }
    FORCEINLINE void SetLayout(VkImageLayout newLayout)
    {
        m_Layout                          = newLayout;
        m_DescriptorImageInfo.imageLayout = m_Layout;
    }

    FORCEINLINE uint32_t GetWidth() const final override { return m_Specification.Width; }
    FORCEINLINE uint32_t GetHeight() const final override { return m_Specification.Height; }
    FORCEINLINE float GetAspectRatio() const final override
    {
        GNT_ASSERT(GetHeight() != 0, "Zero divison!");
        return static_cast<float>(GetWidth() / GetHeight());
    }

    FORCEINLINE const auto& GetDescriptorInfo() const { return m_DescriptorImageInfo; }
    FORCEINLINE void* GetTextureID() const final override
    {
        if (!m_DescriptorSet.Handle) LOG_WARN("Attempting to return NULL image descriptor set!");
        return m_DescriptorSet.Handle;
    }

  private:
    ImageSpecification m_Specification;

    AllocatedImage m_Image;
    VkSampler m_Sampler    = VK_NULL_HANDLE;
    VkImageLayout m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkDescriptorImageInfo m_DescriptorImageInfo;
    DescriptorSet m_DescriptorSet;

    void CreateSampler();
};

class VulkanSamplerStorage final : public SamplerStorage
{
  public:
    VulkanSamplerStorage()  = default;
    ~VulkanSamplerStorage() = default;

  protected:
    void InitializeImpl() final override;
    const Sampler& CreateSamplerImpl(const SamplerSpecification& samplerSpec) final override;
    void DestroyImpl() final override;

    void DestroySamplerImpl(void* handle) final override;
};

}  // namespace Gauntlet