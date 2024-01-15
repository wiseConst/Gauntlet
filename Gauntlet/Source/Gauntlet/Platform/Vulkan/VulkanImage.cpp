#include "GauntletPCH.h"
#include "VulkanImage.h"

#include "VulkanUtility.h"
#include "VulkanContext.h"
#include "VulkanAllocator.h"
#include "VulkanDevice.h"
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

    VkImageCreateInfo imageCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageCreateInfo.imageType         = VK_IMAGE_TYPE_2D;

    imageCreateInfo.extent.width  = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth  = 1;

    imageCreateInfo.mipLevels   = mipLevels;
    imageCreateInfo.arrayLayers = arrayLayers;
    imageCreateInfo.format      = format;
    imageCreateInfo.flags       = arrayLayers == 6 ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

    imageCreateInfo.tiling        = imageTiling;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage         = imageUsageFlags;
    imageCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

    image->Allocation = Context.GetAllocator()->CreateImage(imageCreateInfo, &image->Image);
}

void CreateImageView(const VkDevice& device, const VkImage& image, VkImageView* imageView, VkFormat format, VkImageAspectFlags aspectFlags,
                     VkImageViewType imageViewType, const uint32_t mipLevels)
{
    VkImageViewCreateInfo imageViewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    imageViewCreateInfo.viewType              = imageViewType;
    imageViewCreateInfo.image                 = image;
    imageViewCreateInfo.format                = format;

    // It determines what is affected by your image operation. (In example you are using this image for depth then you set
    // VK_IMAGE_ASPECT_DEPTH_BIT)
    imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;

    imageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    imageViewCreateInfo.subresourceRange.levelCount     = mipLevels;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount     = imageViewType == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;

    // We don't need to swizzle ( swap around ) any of the
    // color channels
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;

    VK_CHECK(vkCreateImageView(device, &imageViewCreateInfo, nullptr, imageView), "Failed to create an image view!");
}

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
        case EImageFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
        case EImageFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case EImageFormat::RGB16F: return VK_FORMAT_R16G16B16_SFLOAT;
        case EImageFormat::RGB32F: return VK_FORMAT_R32G32B32_SFLOAT;
        case EImageFormat::R8: return VK_FORMAT_R8_UNORM;
        case EImageFormat::R16: return VK_FORMAT_R16_UNORM;
        case EImageFormat::R16F: return VK_FORMAT_R16_SFLOAT;
        case EImageFormat::R32F: return VK_FORMAT_R32_SFLOAT;
        case EImageFormat::R11G11B10: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
        case EImageFormat::DEPTH32F: return VK_FORMAT_D32_SFLOAT;
        case EImageFormat::DEPTH24STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;
    }

    GNT_ASSERT(false, "Unknown Image Format!");
    return (VkFormat)0;
}

void TransitionImageLayout(VkImage& image, VkImageLayout oldLayout, VkImageLayout newLayout, EImageFormat format, const uint32_t mipLevels,
                           const bool bIsCubeMap)
{
    GRAPHICS_GUARD_LOCK;

    VkImageSubresourceRange SubresourceRange = {};
    SubresourceRange.aspectMask              = IsDepthFormat(format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    SubresourceRange.layerCount              = bIsCubeMap ? 6 : 1;
    SubresourceRange.levelCount              = mipLevels;

    VkImageMemoryBarrier imageMemoryBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    imageMemoryBarrier.image                = image;
    imageMemoryBarrier.oldLayout            = oldLayout;
    imageMemoryBarrier.newLayout            = newLayout;
    imageMemoryBarrier.subresourceRange     = SubresourceRange;

    // Ownership things...
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

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
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        PipelineSourceStageFlags      = VK_PIPELINE_STAGE_TRANSFER_BIT;
        PipelineDestinationStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        // Shader reads should wait on transfer writes, specifically the shader reads in the fragment shader, because that's
        // where we're going to use the texture.
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    {
        PipelineSourceStageFlags      = VK_PIPELINE_STAGE_TRANSFER_BIT;
        PipelineDestinationStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;

        // Shader reads should wait on transfer writes, specifically the shader reads in the fragment shader, because that's
        // where we're going to use the texture.
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        PipelineSourceStageFlags      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;  // The very beginning of pipeline
        PipelineDestinationStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
    {
        PipelineSourceStageFlags      = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        PipelineDestinationStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        PipelineSourceStageFlags      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;  // The very beginning of pipeline
        PipelineDestinationStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    else
    {
        GNT_ASSERT(false, "Unsupported image layout transition!");
    }

    auto& Context = (VulkanContext&)VulkanContext::Get();

    Ref<VulkanCommandBuffer> commandBuffer = MakeRef<VulkanCommandBuffer>(ECommandBufferType::COMMAND_BUFFER_TYPE_GRAPHICS);
    commandBuffer->BeginRecording(true);

    /*
     * PipelineBarrier
     * Second parameter specifies in which pipeline stage the operations occur that should happen before the barrier. (stages that should be
     * completed before barrier) Third parameter specifies in which pipeline stage the operations occur that should wait on the barrier.
     * (stages that should be completed after barrier)
     */
    commandBuffer->InsertBarrier(PipelineSourceStageFlags, PipelineDestinationStageFlags, VK_DEPENDENCY_BY_REGION_BIT, 0, VK_NULL_HANDLE, 0,
                                 VK_NULL_HANDLE, 1, &imageMemoryBarrier);

    commandBuffer->EndRecording();
    commandBuffer->Submit();
}

void CopyBufferDataToImage(const VkBuffer& sourceBuffer, VkImage& destinationImage, const VkExtent3D& imageExtent, const bool bIsCubeMap)
{
    GRAPHICS_GUARD_LOCK;
    auto& Context = (VulkanContext&)VulkanContext::Get();

    Ref<VulkanCommandBuffer> commandBuffer = MakeRef<VulkanCommandBuffer>(ECommandBufferType::COMMAND_BUFFER_TYPE_TRANSFER);
    commandBuffer->BeginRecording(true);

    VkBufferImageCopy copyRegion           = {};
    copyRegion.imageSubresource.layerCount = bIsCubeMap ? 6 : 1;
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageExtent                 = imageExtent;

    commandBuffer->CopyBufferToImage(sourceBuffer, destinationImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
    commandBuffer->EndRecording();
    commandBuffer->Submit();
}

void GenerateMipmaps(VkImage& image, const VkFormat format, const VkFilter filter, const uint32_t width, const uint32_t height,
                     const uint32_t mipLevels)
{
    GRAPHICS_GUARD_LOCK;

    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    // Check if image format supports linear blitting
    {
        VkFormatProperties formatProperties = {};
        vkGetPhysicalDeviceFormatProperties(context.GetDevice()->GetPhysicalDevice(), format, &formatProperties);

        if (filter == VK_FILTER_LINEAR)
            GNT_ASSERT((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT),
                       "Linear blitting is not supported");
    }

    Ref<VulkanCommandBuffer> commandBuffer = MakeRef<VulkanCommandBuffer>(ECommandBufferType::COMMAND_BUFFER_TYPE_GRAPHICS);
    commandBuffer->BeginRecording(true);

    VkImageMemoryBarrier barrier            = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.image                           = image;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.subresourceRange.levelCount     = 1;

    int32_t mipWidth  = static_cast<int32_t>(width);
    int32_t mipHeight = static_cast<int32_t>(height);
    for (uint32_t i = 1; i < mipLevels; ++i)
    {
        // Inserting first barrier to wait for TRANSITION to SRC_optimal then BLIT
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;

        barrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        commandBuffer->InsertBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                                     &barrier);

        // Downscaling image
        VkImageBlit blit                   = {};
        blit.srcOffsets[0]                 = {0, 0, 0};
        blit.srcOffsets[1]                 = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel       = i - 1;  // Prev
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount     = 1;

        blit.dstOffsets[0]                 = {0, 0, 0};
        blit.dstOffsets[1]                 = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel       = i;  // Current
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount     = 1;

        // Note that image is used for both the srcImage and dstImage parameter.
        // This is because we’re blitting between different levels of the same image
        commandBuffer->BlitImage(image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                                 filter);

        // Inserting second barrier to properly transition our downsampled image into SHADER_READ_ONLY layout
        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        commandBuffer->InsertBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                                     &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    // Last mip case.
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

    commandBuffer->InsertBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                                 &barrier);

    commandBuffer->EndRecording();
    commandBuffer->Submit();
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
        case ETextureWrap::CLAMP_TO_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case ETextureWrap::CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
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
    if (m_Image.Image) Destroy();

    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");

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

    if (ImageUtils::IsDepthFormat(m_Specification.Format)) GNT_ASSERT(m_Specification.Mips == 1, "Depth image cannot have mips!");

    m_Specification.FlipOnLoad = true;
    const auto ImageFormat     = ImageUtils::GauntletImageFormatToVulkan(m_Specification.Format);
    if (ImageUtils::IsDepthFormat(m_Specification.Format))
    {
        GNT_ASSERT(context.GetDevice()->IsDepthFormatSupported(ImageFormat), "Unsupported depth format!");
    }

    ImageUtils::CreateImage(&m_Image, m_Specification.Width, m_Specification.Height, ImageUsageFlags, ImageFormat, VK_IMAGE_TILING_OPTIMAL,
                            m_Specification.Mips, m_Specification.Layers);
    ImageUtils::CreateImageView(context.GetDevice()->GetLogicalDevice(), m_Image.Image, &m_Image.ImageView, ImageFormat,
                                ImageUtils::IsDepthFormat(m_Specification.Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
                                m_Specification.Layers == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D, m_Specification.Mips);

    CreateSampler();

    m_DescriptorImageInfo.imageView = m_Image.ImageView;
    m_DescriptorImageInfo.sampler   = m_Sampler;

    ImageUtils::TransitionImageLayout(m_Image.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                      m_Specification.Format, m_Specification.Mips);

    SetLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    if (m_Specification.CreateTextureID)
    {
        if (!m_DescriptorSet.Handle)  // Preventing allocating on image resizing, just simply update descriptor set
        {
            GNT_ASSERT(context.GetDescriptorAllocator()->Allocate(m_DescriptorSet,
                                                                  VulkanRenderer::GetVulkanStorageData().ImageDescriptorSetLayout),
                       "Failed to allocate texture/image descriptor set!");
        }

        auto TextureWriteDescriptorSet =
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_DescriptorSet.Handle, 1, &m_DescriptorImageInfo);
        vkUpdateDescriptorSets(context.GetDevice()->GetLogicalDevice(), 1, &TextureWriteDescriptorSet, 0, nullptr);
    }

    /*if (m_Specification.Usage == EImageUsage::Attachment)
    {
        const bool bIsDepthFormat = ImageUtils::IsDepthFormat(m_Specification.Format);
        ImageUtils::TransitionImageLayout(m_Image.Image, VK_IMAGE_LAYOUT_UNDEFINED,
                                          bIsDepthFormat ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
                                                         : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                          m_Specification.Format, m_Specification.Mips);

        SetLayout(bIsDepthFormat ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }*/
}

void VulkanImage::CreateSampler()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    SamplerSpecification samplerSpec = {};
    samplerSpec.MinFilter            = m_Specification.Filter;
    samplerSpec.MagFilter            = m_Specification.Filter;
    samplerSpec.AddressModeU = samplerSpec.AddressModeV = samplerSpec.AddressModeW = m_Specification.Wrap;
    samplerSpec.AnisotropyEnable                                                   = true;
    samplerSpec.MaxAnisotropy = context.GetDevice()->GetGPUProperties().limits.maxSamplerAnisotropy;
    samplerSpec.CompareEnable = false;
    samplerSpec.MipmapMode    = m_Specification.Filter;
    samplerSpec.MipLodBias    = 0.05f;
    samplerSpec.MaxLod        = static_cast<float>(m_Specification.Mips);

    m_Sampler = (VkSampler)SamplerStorage::CreateSampler(samplerSpec).Handle;
}

void VulkanImage::Destroy()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    {
        GRAPHICS_GUARD_LOCK;
        context.WaitDeviceOnFinish();
    }

    SamplerStorage::DecrementSamplerRef(m_Sampler);
    if (m_DescriptorSet.Handle) context.GetDescriptorAllocator()->ReleaseDescriptorSets(&m_DescriptorSet, 1);

    context.GetAllocator()->DestroyImage(m_Image.Image, m_Image.Allocation);

    vkDestroyImageView(context.GetDevice()->GetLogicalDevice(), m_Image.ImageView, nullptr);
}

void VulkanSamplerStorage::InitializeImpl()
{
    auto& context     = (VulkanContext&)VulkanContext::Get();
    s_MaxSamplerCount = context.GetDevice()->GetGPUProperties().limits.maxSamplerAllocationCount;
}

const Sampler& VulkanSamplerStorage::CreateSamplerImpl(const SamplerSpecification& samplerSpec)
{
    VkSamplerCreateInfo samplerCreateInfo     = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

    samplerCreateInfo.minFilter = ImageUtils::GauntletTextureFilterToVulkan(samplerSpec.MinFilter);
    samplerCreateInfo.magFilter = ImageUtils::GauntletTextureFilterToVulkan(samplerSpec.MagFilter);

    samplerCreateInfo.addressModeU = ImageUtils::GauntletTextureWrapToVulkan(samplerSpec.AddressModeU);
    samplerCreateInfo.addressModeV = ImageUtils::GauntletTextureWrapToVulkan(samplerSpec.AddressModeV);
    samplerCreateInfo.addressModeW = ImageUtils::GauntletTextureWrapToVulkan(samplerSpec.AddressModeW);

    samplerCreateInfo.anisotropyEnable = samplerSpec.AnisotropyEnable;
    samplerCreateInfo.maxAnisotropy    = samplerSpec.MaxAnisotropy;
    samplerCreateInfo.borderColor      = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCreateInfo.compareEnable    = samplerSpec.CompareEnable;
    samplerCreateInfo.compareOp        = Utility::GauntletCompareOpToVulkan(samplerSpec.CompareOp);
    samplerCreateInfo.mipmapMode =
        samplerSpec.MipmapMode == ETextureFilter::LINEAR ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerCreateInfo.mipLodBias = samplerSpec.MipLodBias;
    samplerCreateInfo.minLod     = samplerSpec.MinLod;
    samplerCreateInfo.maxLod     = samplerSpec.MaxLod;

    for (auto& sampler : s_Samplers)
    {
        if (sampler.first.MinFilter == samplerSpec.MinFilter &&  //
            sampler.first.MagFilter == samplerSpec.MagFilter &&  //

            sampler.first.AddressModeU == samplerSpec.AddressModeU &&  //
            sampler.first.AddressModeV == samplerSpec.AddressModeV &&  //
            sampler.first.AddressModeW == samplerSpec.AddressModeW &&  //

            sampler.first.AnisotropyEnable == samplerSpec.AnisotropyEnable &&  //
            sampler.first.MaxAnisotropy == samplerSpec.MaxAnisotropy &&        //

            sampler.first.CompareEnable == samplerSpec.CompareEnable &&  //
            sampler.first.CompareOp == samplerSpec.CompareOp &&          //

            sampler.first.MipmapMode == samplerSpec.MipmapMode &&  //
            sampler.first.MipLodBias == samplerSpec.MipLodBias &&  //
            sampler.first.MinLod == samplerSpec.MinLod &&          //
            sampler.first.MaxLod == samplerSpec.MaxLod)
        {
            ++sampler.second.ImageUsedCount;
            return sampler.second;
        }
    }

    {
        auto& context = (VulkanContext&)VulkanContext::Get();

        Sampler sampler  = {};
        VkSampler handle = (VkSampler)sampler.Handle;
        VK_CHECK(vkCreateSampler(context.GetDevice()->GetLogicalDevice(), &samplerCreateInfo, nullptr, &handle),
                 "Failed to create an image sampler!");

        sampler.Handle          = handle;
        s_Samplers[samplerSpec] = sampler;
    }
    ++Renderer::GetStats().SamplerCount;

    ++s_Samplers[samplerSpec].ImageUsedCount;
    return s_Samplers[samplerSpec];
}

void VulkanSamplerStorage::DestroyImpl()
{
    auto& context = (VulkanContext&)VulkanContext::Get();

    for (auto& sampler : s_Samplers)
    {
        if (sampler.second.ImageUsedCount != 0)
            vkDestroySampler(context.GetDevice()->GetLogicalDevice(), (VkSampler)sampler.second.Handle, nullptr);

        sampler.second.ImageUsedCount = 0;
        --Renderer::GetStats().SamplerCount;
    }
}

void VulkanSamplerStorage::DestroySamplerImpl(void* handle)
{
    auto& context = (VulkanContext&)VulkanContext::Get();

    vkDestroySampler(context.GetDevice()->GetLogicalDevice(), (VkSampler)handle, nullptr);

    --Renderer::GetStats().SamplerCount;
}

}  // namespace Gauntlet