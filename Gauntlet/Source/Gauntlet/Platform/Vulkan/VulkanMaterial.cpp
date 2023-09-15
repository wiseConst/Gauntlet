#include "GauntletPCH.h"
#include "VulkanMaterial.h"

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanRenderer.h"
#include "VulkanTexture.h"
#include "VulkanTextureCube.h"
#include "VulkanDevice.h"
#include "VulkanUtility.h"
#include "Gauntlet/Renderer/Skybox.h"

namespace Gauntlet
{
// TODO: Refactor

VulkanMaterial::VulkanMaterial()
{
    // Invalidate();
}

void VulkanMaterial::Invalidate()
{
    auto& Context                   = (VulkanContext&)VulkanContext::Get();
    const auto& RendererStorageData = VulkanRenderer::GetStorageData();

    if (!m_DescriptorSet.Handle)
    {
        GNT_ASSERT(Context.GetDescriptorAllocator()->Allocate(m_DescriptorSet,
                                                              RendererStorageData.DefferedDescriptorSetLayout /*MeshDescriptorSetLayout*/),
                   "Failed to allocate descriptor sets!");
    }

    auto WhiteImageInfo = RendererStorageData.MeshWhiteTexture->GetImageDescriptorInfo();
    auto WhiteTextureWriteSet =
        Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_DescriptorSet.Handle, 1, &WhiteImageInfo);

    std::vector<VkWriteDescriptorSet> Writes;
    if (!m_DiffuseTextures.empty())
    {
        auto DiffuseImageInfo = std::static_pointer_cast<VulkanTexture2D>(m_DiffuseTextures[0])->GetImageDescriptorInfo();
        const auto DiffuseTextureWriteSet =
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_DescriptorSet.Handle, 1, &DiffuseImageInfo);

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
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, m_DescriptorSet.Handle, 1, &NormalMapImageInfo);

        Writes.push_back(NormalTextureWriteSet);
    }
    else
    {
        WhiteTextureWriteSet.dstBinding = 1;
        Writes.push_back(WhiteTextureWriteSet);
    }

#if 0
    if (!m_EmissiveTextures.empty())
    {
        auto EmissiveImageInfo = std::static_pointer_cast<VulkanTexture2D>(m_EmissiveTextures[0])->GetImageDescriptorInfo();
        const auto EmissiveTextureWriteSet =
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, m_DescriptorSet.Handle, 1, &EmissiveImageInfo);

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
        auto CubeMapTextureImageInfo      = CubeMapTexture->GetImage()->GetDescriptorInfo();
        const auto CubeMapTextureWriteSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3,
                                                                           m_DescriptorSet.Handle, 1, &CubeMapTextureImageInfo);

        Writes.push_back(CubeMapTextureWriteSet);
    }
    else
    {
        WhiteTextureWriteSet.dstBinding = 3;
        Writes.push_back(WhiteTextureWriteSet);
    }
#endif

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        VkDescriptorBufferInfo CameraDataBufferInfo = {};
        CameraDataBufferInfo.buffer                 = RendererStorageData.UniformCameraDataBuffers[i].Buffer;
        CameraDataBufferInfo.range                  = sizeof(CameraDataBuffer);
        CameraDataBufferInfo.offset                 = 0;

        // CameraWrite
        auto CameraDataBufferWriteSet =
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 /*4*/, m_DescriptorSet.Handle, 1, &CameraDataBufferInfo);

        Writes.push_back(CameraDataBufferWriteSet);
        vkUpdateDescriptorSets(Context.GetDevice()->GetLogicalDevice(), static_cast<uint32_t>(Writes.size()), Writes.data(), 0,
                               VK_NULL_HANDLE);
#if 0
        VkDescriptorBufferInfo LightingModelBufferInfo = {};
        LightingModelBufferInfo.buffer                 = RendererStorageData.UniformPhongModelBuffers[i].Buffer;
        LightingModelBufferInfo.range                  = sizeof(LightingModelBuffer);
        LightingModelBufferInfo.offset                 = 0;

        // PhongModel
        auto PhongModelBufferWriteSet =
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5, m_DescriptorSet.Handle, 1, &LightingModelBufferInfo);

        Writes.push_back(PhongModelBufferWriteSet);

        // Shadows
        if (auto ShadowMapImage = RendererStorageData.ShadowMapFramebuffer->GetDepthAttachment())
        {
            auto ShadowMapImageInfo             = ShadowMapImage->GetDescriptorInfo();
            const auto ShadowMapTextureWriteSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6,
                                                                                 m_DescriptorSet.Handle, 1, &ShadowMapImageInfo);

            Writes.push_back(ShadowMapTextureWriteSet);
        }
        else
        {
            WhiteTextureWriteSet.dstBinding = 6;
            Writes.push_back(WhiteTextureWriteSet);
        }

        VkDescriptorBufferInfo ShadowsBufferInfo = {};
        ShadowsBufferInfo.buffer                 = RendererStorageData.UniformShadowsBuffers[CurrentFrameIndex].Buffer;
        ShadowsBufferInfo.range                  = sizeof(ShadowsBuffer);
        ShadowsBufferInfo.offset                 = 0;

        // Shadows
        auto ShadowsBufferWriteSet =
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 7, m_DescriptorSet.Handle, 1, &ShadowsBufferInfo);
        Writes.push_back(ShadowsBufferWriteSet);

        vkUpdateDescriptorSets(Context.GetDevice()->GetLogicalDevice(), static_cast<uint32_t>(Writes.size()), Writes.data(), 0,
                               VK_NULL_HANDLE);

        Writes.pop_back();
        Writes.pop_back();
        Writes.pop_back();
#endif

        Writes.pop_back();
    }
}

void VulkanMaterial::Destroy()
{
    // TODO: Refactor all of this

    auto& context = (VulkanContext&)VulkanContext::Get();
    context.GetDescriptorAllocator()->ReleaseDescriptorSets(&m_DescriptorSet, 1);
}

}  // namespace Gauntlet