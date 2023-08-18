#include "GauntletPCH.h"
#include "VulkanMaterial.h"

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanDescriptors.h"
#include "VulkanRenderer.h"
#include "VulkanTexture.h"
#include "VulkanTextureCube.h"
#include "VulkanDevice.h"
#include "VulkanUtility.h"
#include "Gauntlet/Renderer/Skybox.h"

namespace Gauntlet
{
VulkanMaterial::VulkanMaterial()
{
    // Invalidate();
}

void VulkanMaterial::Invalidate()
{
    auto& Context                   = (VulkanContext&)VulkanContext::Get();
    const auto& RendererStorageData = VulkanRenderer::GetStorageData();

    if (!m_DescriptorSet)
    {
        GNT_ASSERT(Context.GetDescriptorAllocator()->Allocate(&m_DescriptorSet, RendererStorageData.MeshDescriptorSetLayout),
                   "Failed to allocate descriptor sets!");
    }

    auto WhiteImageInfo = RendererStorageData.MeshWhiteTexture->GetImageDescriptorInfo();
    auto WhiteTextureWriteSet =
        Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_DescriptorSet, 1, &WhiteImageInfo);

    std::vector<VkWriteDescriptorSet> Writes;
    if (!m_DiffuseTextures.empty())
    {
        auto DiffuseImageInfo = std::static_pointer_cast<VulkanTexture2D>(m_DiffuseTextures[0])->GetImageDescriptorInfo();
        const auto DiffuseTextureWriteSet =
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_DescriptorSet, 1, &DiffuseImageInfo);

        Writes.push_back(DiffuseTextureWriteSet);
    }
    else
    {
        WhiteTextureWriteSet.dstBinding = 0;
        Writes.push_back(WhiteTextureWriteSet);
    }

    if (!m_NormalMapTextures.empty())
    {
        auto NormalMapImageInfo = std::static_pointer_cast<VulkanTexture2D>(m_NormalMapTextures[0])->GetImageDescriptorInfo();
        const auto NormalTextureWriteSet =
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, m_DescriptorSet, 1, &NormalMapImageInfo);

        Writes.push_back(NormalTextureWriteSet);
    }
    else
    {
        WhiteTextureWriteSet.dstBinding = 1;
        Writes.push_back(WhiteTextureWriteSet);
    }

    if (!m_EmissiveTextures.empty())
    {
        auto EmissiveImageInfo = std::static_pointer_cast<VulkanTexture2D>(m_EmissiveTextures[0])->GetImageDescriptorInfo();
        const auto EmissiveTextureWriteSet =
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, m_DescriptorSet, 1, &EmissiveImageInfo);

        Writes.push_back(EmissiveTextureWriteSet);
    }
    else
    {
        WhiteTextureWriteSet.dstBinding = 2;
        Writes.push_back(WhiteTextureWriteSet);
    }

    // Environment Map Texture
    if (auto CubeMapTexture = std::static_pointer_cast<VulkanTextureCube>(RendererStorageData.DefaultSkybox->GetCubeMapTexture()))
    {
        auto CubeMapTextureImageInfo = CubeMapTexture->GetImage()->GetDescriptorInfo();
        const auto CubeMapTextureWriteSet =
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, m_DescriptorSet, 1, &CubeMapTextureImageInfo);

        Writes.push_back(CubeMapTextureWriteSet);
    }
    else
    {
        WhiteTextureWriteSet.dstBinding = 3;
        Writes.push_back(WhiteTextureWriteSet);
    }

    const uint32_t CurrentFrameIndex = Context.GetSwapchain()->GetCurrentFrameIndex();
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        VkDescriptorBufferInfo CameraDataBufferInfo = {};
        CameraDataBufferInfo.buffer                 = RendererStorageData.UniformCameraDataBuffers[CurrentFrameIndex].Buffer;
        CameraDataBufferInfo.range                  = sizeof(CameraDataBuffer);
        CameraDataBufferInfo.offset                 = 0;

        // CameraWrite
        auto CameraDataBufferWriteSet =
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4, m_DescriptorSet, 1, &CameraDataBufferInfo);

        Writes.push_back(CameraDataBufferWriteSet);

        VkDescriptorBufferInfo PhongModelBufferInfo = {};
        PhongModelBufferInfo.buffer                 = RendererStorageData.UniformPhongModelBuffers[CurrentFrameIndex].Buffer;
        PhongModelBufferInfo.range                  = sizeof(PhongModelBuffer);
        PhongModelBufferInfo.offset                 = 0;

        // PhongModel
        auto PhongModelBufferWriteSet =
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5, m_DescriptorSet, 1, &PhongModelBufferInfo);

        Writes.push_back(PhongModelBufferWriteSet);

        vkUpdateDescriptorSets(Context.GetDevice()->GetLogicalDevice(), static_cast<uint32_t>(Writes.size()), Writes.data(), 0,
                               VK_NULL_HANDLE);

        Writes.pop_back();
        Writes.pop_back();
    }
}

void VulkanMaterial::Destroy()
{
    // TODO: Refactor all of this
}

}  // namespace Gauntlet