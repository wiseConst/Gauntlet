#include "EclipsePCH.h"
#include "VulkanTexture.h"

#include "VulkanCore.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"

namespace Eclipse
{
// Image

VulkanImage::VulkanImage(const ImageSpecification& InImageSpecification) : m_ImageSpecification(InImageSpecification)
{
    Create();
}

void VulkanImage::Create()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");
    ELS_ASSERT(Context.GetSwapchain()->IsValid(), "Vulkan swapchain is not valid!");

    m_ImageSpecification.Extent.width = Context.GetSwapchain()->GetImageExtent().width;
    m_ImageSpecification.Extent.height = Context.GetSwapchain()->GetImageExtent().height;

    ELS_ASSERT(m_ImageSpecification.ImageViewFormat != VK_FORMAT_UNDEFINED, "Image view format is undefined!");
    ELS_ASSERT(m_ImageSpecification.Format != VK_FORMAT_UNDEFINED, "Image format is undefined!");

    VkImageCreateInfo ImageCreateInfo = {};
    ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCreateInfo.extent = m_ImageSpecification.Extent;
    ImageCreateInfo.format = m_ImageSpecification.Format;
    ImageCreateInfo.imageType = m_ImageSpecification.ImageType;
    ImageCreateInfo.samples = m_ImageSpecification.Samples;
    ImageCreateInfo.tiling = m_ImageSpecification.Tiling;
    ImageCreateInfo.arrayLayers = m_ImageSpecification.ArrayLayers;
    ImageCreateInfo.mipLevels = m_ImageSpecification.MipLevels;
    ImageCreateInfo.usage = m_ImageSpecification.Usage;

    m_AllocatedImage.Allocation = Context.GetAllocator()->CreateImage(ImageCreateInfo, &m_AllocatedImage.Image);

    CreateImageView(Context.GetDevice()->GetLogicalDevice(), m_AllocatedImage.Image, &m_ImageView, m_ImageSpecification.ImageViewFormat,
                    m_ImageSpecification.ImageViewAspectFlags);
}

void VulkanImage::Destroy()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    Context.GetAllocator()->DestroyImage(m_AllocatedImage.Image, m_AllocatedImage.Allocation);

    vkDestroyImageView(Context.GetDevice()->GetLogicalDevice(), m_ImageView, nullptr);
}

VkFormat VulkanImage::ChooseSupportedImageFormat(const VkPhysicalDevice& InDevice, const std::vector<VkFormat>& AvailableFormats,
                                                 VkImageTiling InImageTiling, VkFormatFeatureFlags InFormatFeatures)
{
    for (auto& Format : AvailableFormats)
    {
        VkFormatProperties FormatProps = {};
        vkGetPhysicalDeviceFormatProperties(InDevice, Format, &FormatProps);

        if (InImageTiling == VK_IMAGE_TILING_LINEAR && (FormatProps.linearTilingFeatures & InFormatFeatures) == InFormatFeatures)
        {
            return Format;
        }
        else if (InImageTiling == VK_IMAGE_TILING_OPTIMAL && (FormatProps.optimalTilingFeatures & InFormatFeatures) == InFormatFeatures)
        {
            return Format;
        }
    }

    ELS_ASSERT(false, "Failed to find a supported format!");
    return VK_FORMAT_UNDEFINED;
}

void VulkanImage::CreateImageView(const VkDevice& InDevice, const VkImage& InImage, VkImageView* InImageView, VkFormat InFormat,
                                  VkImageAspectFlags InAspectFlags, VkImageViewType InImageViewType)
{

    VkImageViewCreateInfo ImageViewCreateInfo = {};
    ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCreateInfo.viewType = InImageViewType;

    ImageViewCreateInfo.image = InImage;
    ImageViewCreateInfo.format = InFormat;

    // It determines what is affected by your image operation. (In example you are using this image for depth then you set
    // VK_IMAGE_ASPECT_DEPTH_BIT)
    ImageViewCreateInfo.subresourceRange.aspectMask = InAspectFlags;

    ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    ImageViewCreateInfo.subresourceRange.levelCount = 1;
    ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    ImageViewCreateInfo.subresourceRange.layerCount = 1;

    // We don't need to swizzle ( swap around ) any of the
    // color channels
    ImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    ImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    ImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    ImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;

    VK_CHECK(vkCreateImageView(InDevice, &ImageViewCreateInfo, nullptr, InImageView), "Failed to create an image view!");
}

}  // namespace Eclipse