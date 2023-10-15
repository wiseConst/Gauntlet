#include "GauntletPCH.h"
#include "VulkanTexture.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"

namespace Gauntlet
{

VulkanTexture2D::VulkanTexture2D(const std::string_view& textureFilePath, const TextureSpecification& textureSpecification)
    : m_Specification(textureSpecification)
{
    TextureCreateInfo TextureCI = {};
    int32_t Width               = 0;
    int32_t Height              = 0;
    int32_t Channels            = 0;  // Unused cuz vulkan perceives only 4 channels

    TextureCI.Data     = ImageUtils::LoadImageFromFile(textureFilePath.data(), &Width, &Height, &Channels, true);
    TextureCI.Width    = Width;
    TextureCI.Height   = Height;
    TextureCI.DataSize = Width * Height * TextureCI.Channels;

    Create(TextureCI);
}

VulkanTexture2D::VulkanTexture2D(const void* data, const size_t size, const uint32_t imageWidth, const uint32_t imageHeight,
                                 const TextureSpecification& textureSpecification)
    : m_Specification(textureSpecification)
{
    TextureCreateInfo TextureCI = {};
    TextureCI.Data              = data;
    TextureCI.DataSize          = size;
    TextureCI.Width             = imageWidth;
    TextureCI.Height            = imageHeight;

    Create(TextureCI);
}

void VulkanTexture2D::Destroy()
{
    m_Image->Destroy();
}

void VulkanTexture2D::Create(const TextureCreateInfo& textureCreateInfo)
{
    GNT_ASSERT(textureCreateInfo.Data && textureCreateInfo.DataSize > 0, "Not valid texture create info data!");
    auto& Context = (VulkanContext&)VulkanContext::Get();

    // Create staging buffer for image data
    VulkanAllocatedBuffer StagingBuffer = {};
    BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, textureCreateInfo.DataSize, StagingBuffer, VMA_MEMORY_USAGE_CPU_ONLY);

    // Copy image data to staging buffer
    void* Mapped = Context.GetAllocator()->Map(StagingBuffer.Allocation);
    memcpy(Mapped, textureCreateInfo.Data, textureCreateInfo.DataSize);
    Context.GetAllocator()->Unmap(StagingBuffer.Allocation);

    // Simple image creation
    ImageSpecification ImageSpec = {};
    ImageSpec.Format             = m_Specification.Format;
    ImageSpec.Usage              = EImageUsage::TEXTURE;
    ImageSpec.Width              = textureCreateInfo.Width;
    ImageSpec.Height             = textureCreateInfo.Height;
    ImageSpec.Wrap               = m_Specification.Wrap;
    ImageSpec.Filter             = m_Specification.Filter;
    ImageSpec.CreateTextureID    = m_Specification.CreateTextureID;
    ImageSpec.Layers             = 1;

    if (m_Specification.GenerateMips)
    {
        ImageSpec.Mips     = static_cast<uint32_t>(std::floor(std::log2(std::max(textureCreateInfo.Width, textureCreateInfo.Height)))) + 1;
        ImageSpec.Copyable = m_Specification.GenerateMips;
    }
    m_Image = MakeRef<VulkanImage>(ImageSpec);

    // Transitioning image layout to DST_OPTIMAL to copy staging buffer data into GPU image memory && transitioning image layout to make
    // it readable from fragment shader.

    /* Handling transitions
     * Undefined -> transfer destination: transfer writes don't need to wait on anything.
     * Transfer -> shader read-only: shader read-only should wait on transfer writes operations(in our case after fragment) before
     * transitioning
     */
    ImageUtils::TransitionImageLayout(m_Image->Get(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ImageSpec.Mips);
    ImageUtils::CopyBufferDataToImage(StagingBuffer.Buffer, m_Image->Get(), {m_Image->GetWidth(), m_Image->GetHeight(), 1});

    if (ImageSpec.Mips > 1)
    {
        // Then each level will be transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL after the blit command reading from it is
        // finished.
        ImageUtils::GenerateMipmaps(m_Image->Get(), ImageUtils::GauntletImageFormatToVulkan(ImageSpec.Format),
                                    ImageUtils::GauntletTextureFilterToVulkan(ImageSpec.Filter), ImageSpec.Width, ImageSpec.Height,
                                    ImageSpec.Mips);
    }
    else
        ImageUtils::TransitionImageLayout(m_Image->Get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                          ImageSpec.Mips);

    // All data sent from staging buffer, we can delete it.
    BufferUtils::DestroyBuffer(StagingBuffer);
}

}  // namespace Gauntlet