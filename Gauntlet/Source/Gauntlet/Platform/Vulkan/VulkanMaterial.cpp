#include "GauntletPCH.h"
#include "VulkanMaterial.h"

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanRenderer.h"
#include "VulkanTexture.h"
#include "VulkanTextureCube.h"
#include "VulkanDevice.h"
#include "VulkanUtility.h"
#include "VulkanShader.h"
#include "VulkanBuffer.h"

#pragma warning(disable : 4100)

namespace Gauntlet
{

VulkanMaterial::VulkanMaterial()
{
    // Invalidate();
}

void VulkanMaterial::Update()
{
    m_UBMaterial[GraphicsContext::Get().GetCurrentFrameIndex()]->SetData(&m_Data, sizeof(PBRMaterial));
}

void VulkanMaterial::Invalidate()
{
    auto& Context                   = (VulkanContext&)VulkanContext::Get();
    const auto& RendererStorageData = Renderer::GetStorageData();

    for (auto& materialUB : m_UBMaterial)
    {
        if (!materialUB) materialUB = UniformBuffer::Create(sizeof(PBRMaterial));
    }

    // Material
    VkDescriptorSetLayout geometryDescriptorSetLayout =
        std::static_pointer_cast<VulkanShader>(RendererStorageData.GeometryPipeline->GetSpecification().Shader)
            ->GetDescriptorSetLayouts()[0];
    for (auto& descriptorSet : m_DescriptorSet)
    {
        GNT_ASSERT(Context.GetDescriptorAllocator()->Allocate(descriptorSet, geometryDescriptorSetLayout),
                   "Failed to allocate descriptor sets!");
    }
    // if (!m_DescriptorSet.Handle)
    //{
    //     // Material
    //     VkDescriptorSetLayout geometryDescriptorSetLayout =
    //         std::static_pointer_cast<VulkanShader>(RendererStorageData.GeometryPipeline->GetSpecification().Shader)
    //             ->GetDescriptorSetLayouts()[0];

    //    GNT_ASSERT(Context.GetDescriptorAllocator()->Allocate(m_DescriptorSet, geometryDescriptorSetLayout),
    //               "Failed to allocate descriptor sets!");
    //}

    for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
    {
        auto WhiteImageInfo = static_pointer_cast<VulkanTexture2D>(RendererStorageData.WhiteTexture)->GetImageDescriptorInfo();
        auto WhiteTextureWriteSet =
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, m_DescriptorSet[frame].Handle, 1, &WhiteImageInfo);

        std::vector<VkWriteDescriptorSet> Writes;
        if (!m_AlbedoTextures.empty())
        {
            auto DiffuseImageInfo             = std::static_pointer_cast<VulkanTexture2D>(m_AlbedoTextures[0])->GetImageDescriptorInfo();
            const auto DiffuseTextureWriteSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
                                                                               m_DescriptorSet[frame].Handle, 1, &DiffuseImageInfo);

            Writes.push_back(DiffuseTextureWriteSet);
        }
        else
        {
            WhiteTextureWriteSet.dstBinding = 0;
            Writes.push_back(WhiteTextureWriteSet);
        }

        if (!m_NormalTextures.empty())
        {
            auto NormalMapImageInfo          = std::static_pointer_cast<VulkanTexture2D>(m_NormalTextures[0])->GetImageDescriptorInfo();
            const auto NormalTextureWriteSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                                                                              m_DescriptorSet[frame].Handle, 1, &NormalMapImageInfo);

            Writes.push_back(NormalTextureWriteSet);
        }
        else
        {
            WhiteTextureWriteSet.dstBinding = 1;
            Writes.push_back(WhiteTextureWriteSet);
        }

        if (!m_MetallicTextures.empty())
        {
            auto metallicImageInfo             = std::static_pointer_cast<VulkanTexture2D>(m_MetallicTextures[0])->GetImageDescriptorInfo();
            const auto metallicTextureWriteSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2,
                                                                                m_DescriptorSet[frame].Handle, 1, &metallicImageInfo);

            Writes.push_back(metallicTextureWriteSet);
        }
        else
        {
            WhiteTextureWriteSet.dstBinding = 2;
            Writes.push_back(WhiteTextureWriteSet);
        }

        if (!m_RougnessTextures.empty())
        {
            auto roughnessImageInfo = std::static_pointer_cast<VulkanTexture2D>(m_RougnessTextures[0])->GetImageDescriptorInfo();
            const auto roughnessTextureWriteSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3,
                                                                                 m_DescriptorSet[frame].Handle, 1, &roughnessImageInfo);

            Writes.push_back(roughnessTextureWriteSet);
        }
        else
        {
            WhiteTextureWriteSet.dstBinding = 3;
            Writes.push_back(WhiteTextureWriteSet);
        }

        if (!m_AOTextures.empty())
        {
            auto aoImageInfo             = std::static_pointer_cast<VulkanTexture2D>(m_AOTextures[0])->GetImageDescriptorInfo();
            const auto aoTextureWriteSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4,
                                                                          m_DescriptorSet[frame].Handle, 1, &aoImageInfo);

            Writes.push_back(aoTextureWriteSet);
        }
        else
        {
            WhiteTextureWriteSet.dstBinding = 4;
            Writes.push_back(WhiteTextureWriteSet);
        }

        auto vulkanCameraUB = std::static_pointer_cast<VulkanUniformBuffer>(RendererStorageData.CameraUniformBuffer[frame]);

        VkDescriptorBufferInfo CameraDataBufferInfo = {};
        CameraDataBufferInfo.buffer                 = vulkanCameraUB->Get();
        CameraDataBufferInfo.range                  = sizeof(UBCamera);
        CameraDataBufferInfo.offset                 = 0;

        // CameraWrite
        auto CameraDataBufferWriteSet =
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5, m_DescriptorSet[frame].Handle, 1, &CameraDataBufferInfo);

        Writes.push_back(CameraDataBufferWriteSet);

        auto vulkanMaterialUB                         = std::static_pointer_cast<VulkanUniformBuffer>(m_UBMaterial[frame]);
        VkDescriptorBufferInfo MaterialDataBufferInfo = {};
        MaterialDataBufferInfo.buffer                 = vulkanMaterialUB->Get();
        MaterialDataBufferInfo.range                  = sizeof(PBRMaterial);
        MaterialDataBufferInfo.offset                 = 0;

        // MaterialWrite
        auto MaterialDataBufferWriteSet =
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6, m_DescriptorSet[frame].Handle, 1, &MaterialDataBufferInfo);

        Writes.push_back(MaterialDataBufferWriteSet);

        vkUpdateDescriptorSets(Context.GetDevice()->GetLogicalDevice(), static_cast<uint32_t>(Writes.size()), Writes.data(), 0,
                               VK_NULL_HANDLE);
    }
}

void VulkanMaterial::Destroy()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    //  context.GetDescriptorAllocator()->ReleaseDescriptorSets(&m_DescriptorSet, 1);

    context.GetDescriptorAllocator()->ReleaseDescriptorSets(m_DescriptorSet.data(), m_DescriptorSet.size());

    for (auto& materialUB : m_UBMaterial)
    {
        materialUB->Destroy();
    }
}

}  // namespace Gauntlet