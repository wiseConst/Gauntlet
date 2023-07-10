#include "EclipsePCH.h"
#include "VulkanTexture.h"

#include "VulkanCore.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"

namespace Eclipse
{

VulkanTexture2D::VulkanTexture2D(const std::string_view& TextureFilePath)
{
    ELS_ASSERT(TextureFilePath.data(), "Texture file path is not valid!");

    // Get our image data
    int32_t Width = 0;
    int32_t Height = 0;
    int32_t Channels = 0;
    auto* ImageData = ImageUtils::LoadImageFromFile(TextureFilePath, &Width, &Height, &Channels);

    Channels = 4;  // Vulkan perceives only 4 channels
    const VkDeviceSize ImageSize = Width * Height * Channels;
    auto& Context = (VulkanContext&)VulkanContext::Get();

    // Create staging buffer for image data
    AllocatedBuffer StagingBuffer = {};
    BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, ImageSize, StagingBuffer, VMA_MEMORY_USAGE_CPU_ONLY);

    // Copy image data to staging buffer
    void* Mapped = Context.GetAllocator()->Map(StagingBuffer.Allocation);
    memcpy(Mapped, ImageData, ImageSize);
    Context.GetAllocator()->Unmap(StagingBuffer.Allocation);

    // Get rid of image data
    ImageUtils::UnloadImage(ImageData);

    // Simple image creation
    ImageSpecification ImageSpec = {};
    ImageSpec.Usage = EImageUsage::TEXTURE;
    ImageSpec.Height = Height;
    ImageSpec.Width = Width;
    m_Image.reset(new VulkanImage(ImageSpec));

    // Transitioning image layout to DST_OPTIMAL to copy staging buffer data into GPU image memory && transitioning image layout to make it
    // readable from fragment shader.
    ImageUtils::TransitionImageLayout(m_Image->Get(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    ImageUtils::CopyBufferDataToImage(StagingBuffer.Buffer, m_Image->Get(), {m_Image->GetWidth(), m_Image->GetHeight(), 1});
    ImageUtils::TransitionImageLayout(m_Image->Get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // All data sent from staging buffer, we can delete it.
    BufferUtils::DestroyBuffer(StagingBuffer);

    // TODO: Should depend on ImageSpecification
    // Creating image sampler
    VkSamplerCreateInfo SamplerCreateInfo = {};
    SamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

    SamplerCreateInfo.minFilter = VK_FILTER_NEAREST;
    SamplerCreateInfo.magFilter = VK_FILTER_NEAREST;

    SamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    SamplerCreateInfo.anisotropyEnable = VK_TRUE;
    SamplerCreateInfo.maxAnisotropy = Context.GetDevice()->GetGPUProperties().limits.maxSamplerAnisotropy;

    //  The <borderColor> specifies which color is returned when sampling beyond the image with clamp to border addressing mode. It is
    //  possible to return black, white or transparent in either float or int formats. You cannot specify an arbitrary color.
    SamplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    // ShadowPasses only
    // https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing
    SamplerCreateInfo.compareEnable = VK_FALSE;
    SamplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    // Mipmapping
    SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerCreateInfo.mipLodBias = 0.0f;
    SamplerCreateInfo.minLod = 0.0f;
    SamplerCreateInfo.maxLod = 0.0f;

    VK_CHECK(vkCreateSampler(Context.GetDevice()->GetLogicalDevice(), &SamplerCreateInfo, nullptr, &m_Sampler),
             "Failed to create an image sampler!");
}

VulkanTexture2D::VulkanTexture2D(const void* InData, const size_t InDataSize, const uint32_t InImageWidth, const uint32_t InImageHeight)
{
    ELS_ASSERT(InData && InDataSize > 0, "Texture data is not valid!");
    auto& Context = (VulkanContext&)VulkanContext::Get();

    // Create staging buffer for image data
    AllocatedBuffer StagingBuffer = {};
    BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, InDataSize, StagingBuffer, VMA_MEMORY_USAGE_CPU_ONLY);

    // Copy image data to staging buffer
    void* Mapped = Context.GetAllocator()->Map(StagingBuffer.Allocation);
    memcpy(Mapped, InData, InDataSize);
    Context.GetAllocator()->Unmap(StagingBuffer.Allocation);

    // Simple image creation
    ImageSpecification ImageSpec = {};
    ImageSpec.Usage = EImageUsage::TEXTURE;
    ImageSpec.Height = InImageWidth;
    ImageSpec.Width = InImageHeight;
    m_Image.reset(new VulkanImage(ImageSpec));

    // Transitioning image layout to DST_OPTIMAL to copy staging buffer data into GPU image memory && transitioning image layout to make it
    // readable from fragment shader.
    ImageUtils::TransitionImageLayout(m_Image->Get(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    ImageUtils::CopyBufferDataToImage(StagingBuffer.Buffer, m_Image->Get(), {m_Image->GetWidth(), m_Image->GetHeight(), 1});
    ImageUtils::TransitionImageLayout(m_Image->Get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // All data sent from staging buffer, we can delete it.
    BufferUtils::DestroyBuffer(StagingBuffer);

    // Creating image sampler
    VkSamplerCreateInfo SamplerCreateInfo = {};
    SamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

    SamplerCreateInfo.minFilter = VK_FILTER_NEAREST;
    SamplerCreateInfo.magFilter = VK_FILTER_NEAREST;

    SamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    VK_CHECK(vkCreateSampler(Context.GetDevice()->GetLogicalDevice(), &SamplerCreateInfo, nullptr, &m_Sampler),
             "Failed to create an image sampler!");
}

void VulkanTexture2D::Destroy()
{
    m_Image->Destroy();

    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    vkDestroySampler(Context.GetDevice()->GetLogicalDevice(), m_Sampler, nullptr);
}

}  // namespace Eclipse