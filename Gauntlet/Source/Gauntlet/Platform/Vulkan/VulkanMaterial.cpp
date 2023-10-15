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

#pragma warning(disable : 4100)

namespace Gauntlet
{
// TODO: Refactor

VulkanMaterial::VulkanMaterial()
{
    // Invalidate();
}

void VulkanMaterial::Invalidate()
{
    auto& Context                         = (VulkanContext&)VulkanContext::Get();
    const auto& VulkanRendererStorageData = VulkanRenderer::GetVulkanStorageData();
    const auto& RendererStorageData       = Renderer::GetStorageData();

    if (!m_DescriptorSet.Handle)
    {
        GNT_ASSERT(Context.GetDescriptorAllocator()->Allocate(
                       m_DescriptorSet, VulkanRendererStorageData.GeometryDescriptorSetLayout /*MeshDescriptorSetLayout*/),
                   "Failed to allocate descriptor sets!");
    }

    auto WhiteImageInfo = static_pointer_cast<VulkanTexture2D>(RendererStorageData.WhiteTexture)->GetImageDescriptorInfo();
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

    auto vulkanCameraUB = std::static_pointer_cast<VulkanUniformBuffer>(RendererStorageData.CameraUniformBuffer);
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        VkDescriptorBufferInfo CameraDataBufferInfo = {};
        CameraDataBufferInfo.buffer                 = vulkanCameraUB->GetHandles()[i].Buffer;
        CameraDataBufferInfo.range                  = sizeof(UBCamera);
        CameraDataBufferInfo.offset                 = 0;

        // CameraWrite
        auto CameraDataBufferWriteSet =
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, m_DescriptorSet.Handle, 1, &CameraDataBufferInfo);

        Writes.push_back(CameraDataBufferWriteSet);
        vkUpdateDescriptorSets(Context.GetDevice()->GetLogicalDevice(), static_cast<uint32_t>(Writes.size()), Writes.data(), 0,
                               VK_NULL_HANDLE);

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