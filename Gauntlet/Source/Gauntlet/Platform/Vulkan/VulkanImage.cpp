#include "GauntletPCH.h"
#include "VulkanImage.h"

#include "VulkanUtility.h"
#include "VulkanContext.h"
#include "VulkanAllocator.h"
#include "VulkanDevice.h"
#include "VulkanCommandPool.h"
#include "VulkanDescriptors.h"
#include "VulkanRenderer.h"

namespace Gauntlet
{
namespace ImageUtils
{
void CreateImage(AllocatedImage* image, const uint32_t width, const uint32_t height, VkImageUsageFlags imageUsageFlags, VkFormat format,
                 VkImageTiling imageTiling, const uint32_t mipLevels, const uint32_t arrayLayers)
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    VkImageCreateInfo ImageCreateInfo = {};
    ImageCreateInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCreateInfo.imageType         = VK_IMAGE_TYPE_2D;

    ImageCreateInfo.extent.width  = width;
    ImageCreateInfo.extent.height = height;
    ImageCreateInfo.extent.depth  = 1;

    ImageCreateInfo.mipLevels   = mipLevels;
    ImageCreateInfo.arrayLayers = arrayLayers;
    ImageCreateInfo.format      = format;
    ImageCreateInfo.flags       = arrayLayers == 6 ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

    /*
     * The tiling field can have one of two values:
     * 1) VK_IMAGE_TILING_LINEAR: Texels are laid out in row-major order like our pixels array
     * 2) VK_IMAGE_TILING_OPTIMAL: Texels are laid out in an implementation defined order for optimal access
     *
     * Unlike the layout of an image, the tiling mode cannot be changed at a later time. If you want to be able to directly access texels in
     * the memory of the image, then you must use VK_IMAGE_TILING_LINEAR. We will be using a staging buffer instead of a staging image, so
     * this won't be necessary. We will be using VK_IMAGE_TILING_OPTIMAL for efficient access from the shader.
     *
     * The way I understand it: It defines image texel layout in binded memory
     */
    ImageCreateInfo.tiling = imageTiling;

    /*
     * There are only two possible values for the initialLayout of an image:
     * 1) VK_IMAGE_LAYOUT_UNDEFINED: Not usable by the GPU and the very first transition will discard the texels.
     * 2) VK_IMAGE_LAYOUT_PREINITIALIZED: Not usable by the GPU, but the first transition will preserve the texels.
     *
     * There are few situations where it is necessary for the texels to be preserved during the first transition. One example, however,
     * would be if you wanted to use an image as a staging image in combination with the VK_IMAGE_TILING_LINEAR layout. In that case, you'd
     * want to upload the texel data to it and then transition the image to be a transfer source without losing the data. In our case,
     * however, we're first going to transition the image to be a transfer destination and then copy texel data to it from a buffer object,
     * so we don't need this property and can safely use VK_IMAGE_LAYOUT_UNDEFINED.
     */
    ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    ImageCreateInfo.usage       = imageUsageFlags;
    ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ImageCreateInfo.samples     = VK_SAMPLE_COUNT_1_BIT;

    image->Allocation = Context.GetAllocator()->CreateImage(ImageCreateInfo, &image->Image);
}

void CreateImageView(const VkDevice& device, const VkImage& image, VkImageView* imageView, VkFormat format, VkImageAspectFlags aspectFlags,
                     VkImageViewType imageViewType)
{
    VkImageViewCreateInfo ImageViewCreateInfo = {};
    ImageViewCreateInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCreateInfo.viewType              = imageViewType;

    ImageViewCreateInfo.image  = image;
    ImageViewCreateInfo.format = format;

    // It determines what is affected by your image operation. (In example you are using this image for depth then you set
    // VK_IMAGE_ASPECT_DEPTH_BIT)
    ImageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;

    ImageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    ImageViewCreateInfo.subresourceRange.levelCount     = 1;
    ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    ImageViewCreateInfo.subresourceRange.layerCount     = imageViewType == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;

    // We don't need to swizzle ( swap around ) any of the
    // color channels
    ImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    ImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    ImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    ImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;

    VK_CHECK(vkCreateImageView(device, &ImageViewCreateInfo, nullptr, imageView), "Failed to create an image view!");
}

/* Why surface has BGRA && images that we're rendering have RGBA:
 * RGBA is easier to work with since image loading libraries generally store the pixels in that layout,
 * but it's usually not the order that is supported by display surfaces.
 * That's why we use the BGRA order there.
 *
 * Also vulkan handles transition from RGBA to BGRA behind the scenes.
 */
VkFormat GauntletImageFormatToVulkan(EImageFormat imageFormat)
{
    switch (imageFormat)
    {
        case EImageFormat::NONE:
        {
            GNT_ASSERT(false, "Image Format is NONE!");
            break;
        }
        case EImageFormat::RGB: return VK_FORMAT_R8G8B8_UNORM;
        case EImageFormat::RGBA: return VK_FORMAT_R8G8B8A8_UNORM;
        case EImageFormat::SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
        case EImageFormat::DEPTH32F: return VK_FORMAT_D32_SFLOAT;
        case EImageFormat::DEPTH24STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;
    }

    GNT_ASSERT(false, "Unknown Image Format!");
    return (VkFormat)0;
}

void TransitionImageLayout(VkImage& image, VkImageLayout oldLayout, VkImageLayout newLayout, const bool bIsCubeMap)
{
    GRAPHICS_GUARD_LOCK;

    auto& Context = (VulkanContext&)VulkanContext::Get();

    VkImageSubresourceRange SubresourceRange = {};
    SubresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
    SubresourceRange.layerCount              = bIsCubeMap ? 6 : 1;
    SubresourceRange.levelCount              = 1;

    VkImageMemoryBarrier ImageMemoryBarrier = {};
    ImageMemoryBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    ImageMemoryBarrier.image                = image;
    ImageMemoryBarrier.oldLayout            = oldLayout;
    ImageMemoryBarrier.newLayout            = newLayout;
    ImageMemoryBarrier.subresourceRange     = SubresourceRange;

    // Ownership things...
    ImageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    VkPipelineStageFlags PipelineSourceStageFlags      = 0;
    VkPipelineStageFlags PipelineDestinationStageFlags = 0;

    /*
     * The way I understand it right now:
     * You specify srcAccessMask(operation that gonna occur), source pipeline stage(from what stage of pipeline your srcAccessMask should
     * occur) and destination pipeline stage(until which stage srcAccessMask should occur)
     *
     * Then you specify dstAccessMask(operation that gonna occur after source pipeline stage) and destination pipeline stage(from what stage
     * should occur dstAccessMask until the end of pipeline)
     */
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        PipelineSourceStageFlags      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;  // The very beginning of pipeline
        PipelineDestinationStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;

        // Transfer writes that don't need to wait on anything
        ImageMemoryBarrier.srcAccessMask = 0;
        ImageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        PipelineSourceStageFlags      = VK_PIPELINE_STAGE_TRANSFER_BIT;
        PipelineDestinationStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        // Shader reads should wait on transfer writes, specifically the shader reads in the fragment shader, because that's
        // where we're going to use the texture.
        ImageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        ImageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }
    else
    {
        GNT_ASSERT(false, "Unsupported image layout transition!");
    }

    // Crutch.
    // Problem: The transfer queue does not support VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT -- only the graphics queue does.
    const bool bIsFragmentShaderStageInDestination = PipelineDestinationStageFlags == VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    auto CommandBuffer = Utility::BeginSingleTimeCommands(bIsFragmentShaderStageInDestination ? Context.GetGraphicsCommandPool()->Get()
                                                                                              : Context.GetTransferCommandPool()->Get(),
                                                          Context.GetDevice()->GetLogicalDevice());

    /*
     * PipelineBarrier
     * Second parameter specifies in which pipeline stage the operations occur that should happen before the barrier.
     * Third parameter specifies in which pipeline stage the operations occur that should wait on the barrier.
     */
    vkCmdPipelineBarrier(CommandBuffer, PipelineSourceStageFlags, PipelineDestinationStageFlags, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1,
                         &ImageMemoryBarrier);

    Utility::EndSingleTimeCommands(
        CommandBuffer,
        bIsFragmentShaderStageInDestination ? Context.GetGraphicsCommandPool()->Get() : Context.GetTransferCommandPool()->Get(),
        bIsFragmentShaderStageInDestination ? Context.GetDevice()->GetGraphicsQueue() : Context.GetDevice()->GetTransferQueue(),
        Context.GetDevice()->GetLogicalDevice());
}

void CopyBufferDataToImage(const VkBuffer& sourceBuffer, VkImage& destinationImage, const VkExtent3D& imageExtent, const bool bIsCubeMap)
{
    GRAPHICS_GUARD_LOCK;
    auto& Context      = (VulkanContext&)VulkanContext::Get();
    auto CommandBuffer = Utility::BeginSingleTimeCommands(Context.GetTransferCommandPool()->Get(), Context.GetDevice()->GetLogicalDevice());

    VkBufferImageCopy CopyRegion           = {};
    CopyRegion.imageSubresource.layerCount = bIsCubeMap ? 6 : 1;
    CopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    CopyRegion.imageExtent                 = imageExtent;

    vkCmdCopyBufferToImage(CommandBuffer, sourceBuffer, destinationImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &CopyRegion);

    Utility::EndSingleTimeCommands(CommandBuffer, Context.GetTransferCommandPool()->Get(), Context.GetDevice()->GetTransferQueue(),
                                   Context.GetDevice()->GetLogicalDevice());
}

VkFilter GauntletTextureFilterToVulkan(ETextureFilter textureFilter)
{
    switch (textureFilter)
    {
        case ETextureFilter::LINEAR: return VK_FILTER_LINEAR;
        case ETextureFilter::NEAREST: return VK_FILTER_NEAREST;
    }
    GNT_ASSERT(false, "Unknown texture filter!");
    return VK_FILTER_NEAREST;
}

VkSamplerAddressMode GauntletTextureWrapToVulkan(ETextureWrap textureWrap)
{
    switch (textureWrap)
    {
        case ETextureWrap::REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case ETextureWrap::CLAMP: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    }

    GNT_ASSERT(false, "Unknown texture wrap!");
    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

}  // namespace ImageUtils

VulkanImage::VulkanImage(const ImageSpecification& imageSecification) : m_Specification(imageSecification)
{
    Invalidate();
}

void VulkanImage::Invalidate()
{
    if (m_Image.Image)
    {
        Destroy();
    }

    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    VkImageUsageFlags ImageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT;
    if (m_Specification.Copyable) ImageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    if (m_Specification.Usage == EImageUsage::Attachment)
    {
        if (ImageUtils::IsDepthFormat(m_Specification.Format))
        {
            ImageUsageFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        else
        {
            ImageUsageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
    }
    else if (m_Specification.Usage == EImageUsage::TEXTURE)
    {
        ImageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    m_Specification.FlipOnLoad = true;
    const auto ImageFormat     = ImageUtils::GauntletImageFormatToVulkan(m_Specification.Format);
    ImageUtils::CreateImage(&m_Image, m_Specification.Width, m_Specification.Height, ImageUsageFlags, ImageFormat, VK_IMAGE_TILING_OPTIMAL,
                            m_Specification.Mips, m_Specification.Layers);
    ImageUtils::CreateImageView(Context.GetDevice()->GetLogicalDevice(), m_Image.Image, &m_Image.ImageView, ImageFormat,
                                ImageUtils::IsDepthFormat(m_Specification.Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
                                m_Specification.Layers == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D);

    CreateSampler();

    m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    m_DescriptorImageInfo.imageView   = m_Image.ImageView;
    m_DescriptorImageInfo.sampler     = m_Sampler;

    if (m_Specification.CreateTextureID)
    {
        if (!m_DescriptorSet)  // Preventing allocating on image resizing, just simply update descriptor set
        {
            VK_CHECK(
                Context.GetDescriptorAllocator()->Allocate(&m_DescriptorSet, VulkanRenderer::GetStorageData().ImageDescriptorSetLayout),
                "Failed to allocate texture/image descriptor set!");
        }

        auto TextureWriteDescriptorSet =
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_DescriptorSet, 1, &m_DescriptorImageInfo);
        vkUpdateDescriptorSets(Context.GetDevice()->GetLogicalDevice(), 1, &TextureWriteDescriptorSet, 0, nullptr);
    }
}

void VulkanImage::CreateSampler()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    VkSamplerCreateInfo SamplerCreateInfo     = {};
    SamplerCreateInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

    const bool bIsCubeMap       = m_Specification.Layers == 6;
    SamplerCreateInfo.minFilter = bIsCubeMap ? VK_FILTER_LINEAR : ImageUtils::GauntletTextureFilterToVulkan(m_Specification.Filter);
    SamplerCreateInfo.magFilter = bIsCubeMap ? VK_FILTER_LINEAR : ImageUtils::GauntletTextureFilterToVulkan(m_Specification.Filter);

    SamplerCreateInfo.addressModeU =
        bIsCubeMap ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : ImageUtils::GauntletTextureWrapToVulkan(m_Specification.Wrap);
    SamplerCreateInfo.addressModeV =
        bIsCubeMap ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : ImageUtils::GauntletTextureWrapToVulkan(m_Specification.Wrap);
    SamplerCreateInfo.addressModeW =
        bIsCubeMap ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : ImageUtils::GauntletTextureWrapToVulkan(m_Specification.Wrap);

    SamplerCreateInfo.anisotropyEnable = VK_TRUE;
    SamplerCreateInfo.maxAnisotropy    = Context.GetDevice()->GetGPUProperties().limits.maxSamplerAnisotropy;

    //  The <borderColor> specifies which color is returned when sampling beyond the image with clamp to border addressing mode. It is
    //  possible to return black, white or transparent in either float or int formats. You cannot specify an arbitrary color.
    SamplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    // ShadowPasses only
    // https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing
    SamplerCreateInfo.compareEnable = VK_FALSE;
    SamplerCreateInfo.compareOp     = VK_COMPARE_OP_ALWAYS;

    // Mipmapping
    SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerCreateInfo.mipLodBias = 0.0f;
    SamplerCreateInfo.minLod     = 0.0f;
    SamplerCreateInfo.maxLod     = 0.0f;

    VK_CHECK(vkCreateSampler(Context.GetDevice()->GetLogicalDevice(), &SamplerCreateInfo, nullptr, &m_Sampler),
             "Failed to create an image sampler!");
}

void VulkanImage::Destroy()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    Context.GetDevice()->WaitDeviceOnFinish();
    Context.GetAllocator()->DestroyImage(m_Image.Image, m_Image.Allocation);

    vkDestroyImageView(Context.GetDevice()->GetLogicalDevice(), m_Image.ImageView, nullptr);
    vkDestroySampler(Context.GetDevice()->GetLogicalDevice(), m_Sampler, nullptr);
}

}  // namespace Gauntlet