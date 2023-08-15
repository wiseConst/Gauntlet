#include "GauntletPCH.h"
#include "VulkanTexture.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"

namespace Gauntlet
{

VulkanTexture2D::VulkanTexture2D(const std::string_view& TextureFilePath, const bool InbCreateTextureID)
{
    TextureCreateInfo TextureCI = {};
    int32_t Width               = 0;
    int32_t Height              = 0;
    int32_t Channels            = 0;  // Unused cuz vulkan perceives only 4 channels

    TextureCI.Data     = ImageUtils::LoadImageFromFile(TextureFilePath.data(), &Width, &Height, &Channels, true);
    TextureCI.Width    = Width;
    TextureCI.Height   = Height;
    TextureCI.DataSize = Width * Height * TextureCI.Channels;
    TextureCI.CreateTextureID = InbCreateTextureID;

    Create(TextureCI);
}

VulkanTexture2D::VulkanTexture2D(const void* InData, const size_t InDataSize, const uint32_t InImageWidth, const uint32_t InImageHeight)
{
    TextureCreateInfo TextureCI = {};
    TextureCI.Data              = InData;
    TextureCI.Width             = InImageWidth;
    TextureCI.Height            = InImageHeight;
    TextureCI.DataSize          = InDataSize;

    Create(TextureCI);
}

void VulkanTexture2D::Destroy()
{
    m_Image->Destroy();
}

void VulkanTexture2D::Create(const TextureCreateInfo& InTextureCreateInfo)
{
    GNT_ASSERT(InTextureCreateInfo.Data && InTextureCreateInfo.DataSize > 0, "Not valid texture create info data!");
    auto& Context = (VulkanContext&)VulkanContext::Get();

    // Create staging buffer for image data
    AllocatedBuffer StagingBuffer = {};
    BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, InTextureCreateInfo.DataSize, StagingBuffer, VMA_MEMORY_USAGE_CPU_ONLY);

    // Copy image data to staging buffer
    void* Mapped = Context.GetAllocator()->Map(StagingBuffer.Allocation);
    memcpy(Mapped, InTextureCreateInfo.Data, InTextureCreateInfo.DataSize);
    Context.GetAllocator()->Unmap(StagingBuffer.Allocation);

    // Simple image creation
    ImageSpecification ImageSpec = {};
    ImageSpec.Usage              = EImageUsage::TEXTURE;
    ImageSpec.Width              = InTextureCreateInfo.Width;
    ImageSpec.Height             = InTextureCreateInfo.Height;
    ImageSpec.Layers             = 1;
    ImageSpec.CreateTextureID    = InTextureCreateInfo.CreateTextureID;
    m_Image.reset(new VulkanImage(ImageSpec));

    // Transitioning image layout to DST_OPTIMAL to copy staging buffer data into GPU image memory && transitioning image layout to make
    // it readable from fragment shader.
    ImageUtils::TransitionImageLayout(m_Image->Get(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    ImageUtils::CopyBufferDataToImage(StagingBuffer.Buffer, m_Image->Get(), {m_Image->GetWidth(), m_Image->GetHeight(), 1});
    ImageUtils::TransitionImageLayout(m_Image->Get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // All data sent from staging buffer, we can delete it.
    BufferUtils::DestroyBuffer(StagingBuffer);
}

}  // namespace Gauntlet