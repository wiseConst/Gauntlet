#include "GauntletPCH.h"
#include "VulkanTextureCube.h"
#include "VulkanTexture.h"

#include "VulkanContext.h"
#include "VulkanBuffer.h"

namespace Gauntlet
{

constexpr static uint32_t s_MaxCubeMapImages = 6;

VulkanTextureCube::VulkanTextureCube(const std::vector<std::string>& faces) : m_Faces(faces)
{
    GNT_ASSERT(faces.size() == s_MaxCubeMapImages, "Cube maps size should be 6!");
    Create();
}

VulkanTextureCube::VulkanTextureCube(const TextureCubeSpecification& textureCubeSpec) {}

// No TextureCube recreation right now
void VulkanTextureCube::Create()
{
    int32_t Width, Height, Channels;
    uint8_t* FacesData[s_MaxCubeMapImages];

    // Assuming that all the images have the same width && height
    for (uint32_t i = 0; i < s_MaxCubeMapImages; ++i)
    {
        FacesData[i] = ImageUtils::LoadImageFromFile(m_Faces[i].data(), &Width, &Height, &Channels, true);
        GNT_ASSERT(FacesData[i], "Failed to load cube map image!");
    }
    Channels = 4;

    const VkDeviceSize ImageSize   = Width * Height * Channels;
    const VkDeviceSize CubeMapSize = ImageSize * s_MaxCubeMapImages;

    // Create staging buffer for image data
    VulkanAllocatedBuffer StagingBuffer = {};
    BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, CubeMapSize, StagingBuffer, VMA_MEMORY_USAGE_CPU_ONLY);

    const auto& Context = (VulkanContext&)VulkanContext::Get();

    // Copy image data to staging buffer
    void* Mapped = Context.GetAllocator()->Map(StagingBuffer.Allocation);
    for (uint32_t i = 0; i < s_MaxCubeMapImages; ++i)
    {
        void* ExtendedMapped = reinterpret_cast<void*>(reinterpret_cast<uint64_t>(Mapped) + (uint64_t)(ImageSize * i));
        memcpy(ExtendedMapped, FacesData[i], ImageSize);
    }
    Context.GetAllocator()->Unmap(StagingBuffer.Allocation);

    // Simple image creation
    ImageSpecification ImageSpec = {};
    ImageSpec.Usage              = EImageUsage::TEXTURE;
    ImageSpec.Width              = Width;
    ImageSpec.Height             = Height;
    ImageSpec.Layers             = s_MaxCubeMapImages;
    ImageSpec.Mips               = 1;
    m_Image                      = MakeRef<VulkanImage>(ImageSpec);

    // Transitioning image layout to DST_OPTIMAL to copy staging buffer data into GPU image memory && transitioning image layout to make it
    // readable from fragment shader.
    ImageUtils::TransitionImageLayout(m_Image->Get(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ImageSpec.Mips,
                                      true);
    ImageUtils::CopyBufferDataToImage(StagingBuffer.Buffer, m_Image->Get(), {m_Image->GetWidth(), m_Image->GetHeight(), 1}, true);
    ImageUtils::TransitionImageLayout(m_Image->Get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                      ImageSpec.Mips, true);

    // All data sent from staging buffer, we can delete it.
    BufferUtils::DestroyBuffer(StagingBuffer);
    for (uint32_t i = 0; i < s_MaxCubeMapImages; ++i)
    {
        ImageUtils::UnloadImage(FacesData[i]);
    }
}

void VulkanTextureCube::Destroy()
{
    m_Image->Destroy();
}

}  // namespace Gauntlet