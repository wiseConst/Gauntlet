#include "EclipsePCH.h"
#include "VulkanImage.h"

#include "VulkanContext.h"
#include "VulkanAllocator.h"
#include "VulkanDevice.h"

namespace Eclipse
{
namespace ImageUtils
{
void CreateImage(AllocatedImage* InImage, const uint32_t InWidth, const uint32_t InHeight, VkImageUsageFlags InImageUsageFlags,
                 VkFormat InFormat, VkImageTiling InImageTiling, const uint32_t InMipLevels, const uint32_t InArrayLayers)
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    VkImageCreateInfo ImageCreateInfo = {};
    ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageCreateInfo.extent.width = InWidth;
    ImageCreateInfo.extent.height = InHeight;
    ImageCreateInfo.extent.depth = 1;
    ImageCreateInfo.mipLevels = InMipLevels;
    ImageCreateInfo.arrayLayers = InArrayLayers;
    ImageCreateInfo.format = InFormat;
    ImageCreateInfo.tiling = InImageTiling;
    ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageCreateInfo.usage = InImageUsageFlags;
    ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    InImage->Allocation = Context.GetAllocator()->CreateImage(ImageCreateInfo, &InImage->Image);
}

VkFormat ChooseSupportedImageFormat(const VkPhysicalDevice& InDevice, const std::vector<VkFormat>& AvailableFormats,
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

void CreateImageView(const VkDevice& InDevice, const VkImage& InImage, VkImageView* InImageView, VkFormat InFormat,
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

VkFormat EclipseImageFormatToVulkan(EImageFormat InImageFormat)
{
    switch (InImageFormat)
    {
        case EImageFormat::NONE:
        {
            ELS_ASSERT(false, "Image Format is NONE!");
            break;
        }
        case EImageFormat::RGB: VK_FORMAT_B8G8R8_UNORM;
        case EImageFormat::RGBA: VK_FORMAT_B8G8R8A8_UNORM;
        case EImageFormat::SRGB: VK_FORMAT_B8G8R8A8_SRGB;
        case EImageFormat::DEPTH32F: return VK_FORMAT_D32_SFLOAT;
        case EImageFormat::DEPTH24STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;
    }

    ELS_ASSERT(false, "Unknown Image Format!");
    return (VkFormat)0;
}
}  // namespace ImageUtils

VulkanImage::VulkanImage(const ImageSpecification& InImageSecification, const char* InFilePath) : m_ImageSpecification(InImageSecification)
{
    Create();
}

void VulkanImage::Create()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    VkImageUsageFlags ImageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT;
    if (m_ImageSpecification.Copyable) ImageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    if (m_ImageSpecification.Usage == EImageUsage::Attachment)
    {
        if (ImageUtils::IsDepthFormat(m_ImageSpecification.Format))
        {
            ImageUsageFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        else
        {
            ImageUsageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
    }
    else if (m_ImageSpecification.Usage == EImageUsage::TEXTURE)
    {
        ImageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    const auto ImageFormat = ImageUtils::EclipseImageFormatToVulkan(m_ImageSpecification.Format);
    ImageUtils::CreateImage(&m_Image, m_ImageSpecification.Width, m_ImageSpecification.Height, ImageUsageFlags, ImageFormat,
                            VK_IMAGE_TILING_OPTIMAL, m_ImageSpecification.Mips, m_ImageSpecification.Layers);
    ImageUtils::CreateImageView(Context.GetDevice()->GetLogicalDevice(), m_Image.Image, &m_Image.ImageView, ImageFormat,
                                ImageUtils::IsDepthFormat(m_ImageSpecification.Format) ? VK_IMAGE_ASPECT_DEPTH_BIT
                                                                                       : VK_IMAGE_ASPECT_COLOR_BIT,
                                VK_IMAGE_VIEW_TYPE_2D);
}

void VulkanImage::Destroy()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    Context.GetAllocator()->DestroyImage(m_Image.Image, m_Image.Allocation);

    vkDestroyImageView(Context.GetDevice()->GetLogicalDevice(), m_Image.ImageView, nullptr);
    vkDestroySampler(Context.GetDevice()->GetLogicalDevice(), m_Sampler, nullptr);
}

}  // namespace Eclipse