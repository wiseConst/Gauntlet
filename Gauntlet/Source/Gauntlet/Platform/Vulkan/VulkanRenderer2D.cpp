#include "GauntletPCH.h"
#include "VulkanRenderer2D.h"

#include "VulkanContext.h"
#include "VulkanUtility.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanShader.h"
#include "VulkanPipeline.h"

#include "VulkanTexture.h"
#include "VulkanCommandPool.h"
#include "VulkanDescriptors.h"
#include "VulkanFramebuffer.h"
#include "VulkanRenderer.h"

// Refactor

namespace Gauntlet
{
VulkanRenderer2D::VulkanRenderer2D() : m_Context((VulkanContext&)VulkanContext::Get())
{
    Create();
}

void VulkanRenderer2D::Create()
{
    // TODO: Apply normals in 2D

    // Vertex buffer creation
    {
        s_Data2D.VertexBufferLayout = {
            {EShaderDataType::Vec3, "InPostion"},    //
            {EShaderDataType::Vec3, "InNormal"},     //
            {EShaderDataType::Vec4, "InColor"},      //
            {EShaderDataType::Vec2, "InTexCoord"},   //
            {EShaderDataType::FLOAT, "InTextureId"}  //
        };                                           //

        s_Data2D.QuadVertexBufferBase = new QuadVertex[s_Data2D.MaxVertices];
    }

    // Index Buffer creation
    {
        uint32_t* QuadIndices = new uint32_t[s_Data2D.MaxIndices];

        // Clockwise order
        uint32_t Offset = 0;
        for (uint32_t i = 0; i < s_Data2D.MaxIndices; i += 6)
        {
            QuadIndices[i + 0] = Offset + 0;
            QuadIndices[i + 1] = Offset + 1;
            QuadIndices[i + 2] = Offset + 2;

            QuadIndices[i + 3] = Offset + 2;
            QuadIndices[i + 4] = Offset + 3;
            QuadIndices[i + 5] = Offset + 0;

            Offset += 4;
        }

        BufferInfo IndexBufferInfo = {};
        IndexBufferInfo.Usage      = EBufferUsageFlags::INDEX_BUFFER;
        IndexBufferInfo.Count      = s_Data2D.MaxIndices;
        IndexBufferInfo.Size       = s_Data2D.MaxIndices * sizeof(uint32_t);
        IndexBufferInfo.Data       = QuadIndices;
        s_Data2D.QuadIndexBuffer   = MakeRef<VulkanIndexBuffer>(IndexBufferInfo);

        delete[] QuadIndices;
    }

    s_Data2D.TextureSlots[0] = VulkanRenderer::GetStorageData().MeshWhiteTexture;

    // Setting up things all the things that related to uniforms in shader
    const auto TextureBinding = Utility::GetDescriptorSetLayoutBinding(
        0, VulkanRenderer2DStorage::MaxTextureSlots, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

    // Vulkan 1.2 Features for texture batching in my case
    VkDescriptorBindingFlags BindingFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

    VkDescriptorSetLayoutBindingFlagsCreateInfo BindingFlagsInfo = {};
    BindingFlagsInfo.sType                                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    BindingFlagsInfo.bindingCount                                = 1;
    BindingFlagsInfo.pBindingFlags                               = &BindingFlags;

    auto QuadDescriptorSetLayoutCreateInfo = Utility::GetDescriptorSetLayoutCreateInfo(1, &TextureBinding, 0, &BindingFlagsInfo);
    VK_CHECK(vkCreateDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), &QuadDescriptorSetLayoutCreateInfo, nullptr,
                                         &s_Data2D.QuadDescriptorSetLayout),
             "Failed to create descriptor set layout!");

    // Default 2D graphics pipeline creation
    {
        PipelineSpecification PipelineSpec = {};
        PipelineSpec.Name                  = "Quad2D";
        PipelineSpec.TargetFramebuffer     = VulkanRenderer::GetStorageData().GeometryFramebuffer;
        PipelineSpec.bDepthTest            = true;
        PipelineSpec.bDepthWrite           = true;
        PipelineSpec.DepthCompareOp        = VK_COMPARE_OP_LESS;

        const std::vector<VkVertexInputBindingDescription> ShaderBindingDescriptions = {
            Utility::GetShaderBindingDescription(0, s_Data2D.VertexBufferLayout.GetStride())};
        PipelineSpec.ShaderBindingDescriptions = ShaderBindingDescriptions;

        std::vector<VkVertexInputAttributeDescription> ShaderAttributeDescriptions(s_Data2D.VertexBufferLayout.GetElements().size());
        for (uint32_t i = 0; i < ShaderAttributeDescriptions.size(); ++i)
        {
            ShaderAttributeDescriptions[i] = Utility::GetShaderAttributeDescription(
                0, Utility::GauntletShaderDataTypeToVulkan(s_Data2D.VertexBufferLayout.GetElements()[i].Type), i,
                s_Data2D.VertexBufferLayout.GetElements()[i].Offset);
        }

        PipelineSpec.ShaderAttributeDescriptions = ShaderAttributeDescriptions;

        auto VertexShader   = Ref<VulkanShader>(new VulkanShader("Resources/Cached/Shaders/FlatColor.vert.spv"));
        auto FragmentShader = Ref<VulkanShader>(new VulkanShader("Resources/Cached/Shaders/FlatColor.frag.spv"));

        std::vector<PipelineSpecification::ShaderStage> ShaderStages(2);
        ShaderStages[0].Stage  = EShaderStage::SHADER_STAGE_VERTEX;
        ShaderStages[0].Shader = VertexShader;

        ShaderStages[1].Stage  = EShaderStage::SHADER_STAGE_FRAGMENT;
        ShaderStages[1].Shader = FragmentShader;

        PipelineSpec.ShaderStages       = ShaderStages;
        PipelineSpec.PushConstantRanges = {Utility::GetPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(MeshPushConstants))};

        PipelineSpec.DescriptorSetLayouts = {s_Data2D.QuadDescriptorSetLayout};

        s_Data2D.QuadPipeline = MakeRef<VulkanPipeline>(PipelineSpec);

        VertexShader->Destroy();
        FragmentShader->Destroy();
    }

    // Initially 1 staging buffer
    Scoped<VulkanStagingStorage::StagingBuffer> QuadVertexStagingBuffer(new VulkanStagingStorage::StagingBuffer());
    BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, s_Data2D.DefaultVertexBufferSize, (*QuadVertexStagingBuffer).Buffer);
    QuadVertexStagingBuffer->Capacity = s_Data2D.DefaultVertexBufferSize;
    s_Data2D.StagingStorage.StagingBuffers.emplace_back(std::move(QuadVertexStagingBuffer));
    ++Renderer::GetStats().StagingVertexBuffers;

    if (s_Data2D.CurrentDescriptorSetIndex >= s_Data2D.QuadDescriptorSets.size())
    {
        DescriptorSet QuadSamplerDescriptorSet;
        GNT_ASSERT(m_Context.GetDescriptorAllocator()->Allocate(QuadSamplerDescriptorSet, s_Data2D.QuadDescriptorSetLayout),
                   "Failed to allocate descriptor sets!");
        s_Data2D.QuadDescriptorSets.push_back(QuadSamplerDescriptorSet);
    }

    VkDescriptorImageInfo WhiteImageInfo = s_Data2D.TextureSlots[0]->GetImageDescriptorInfo();
    const auto TextureWriteSet =
        Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
                                       s_Data2D.QuadDescriptorSets[s_Data2D.CurrentDescriptorSetIndex].Handle, 1, &WhiteImageInfo);
    vkUpdateDescriptorSets(m_Context.GetDevice()->GetLogicalDevice(), 1, &TextureWriteSet, 0, nullptr);
}

void VulkanRenderer2D::BeginImpl()
{
    // Release unused descriptor sets
    if (s_Data2D.QuadDescriptorSets.size() > 0 && s_Data2D.CurrentDescriptorSetIndex > 0)
    {
        if (s_Data2D.QuadDescriptorSets.size() > s_Data2D.CurrentDescriptorSetIndex)
        {
            const uint64_t descriptorSetsToFree = s_Data2D.QuadDescriptorSets.size() - s_Data2D.CurrentDescriptorSetIndex;
            m_Context.GetDescriptorAllocator()->ReleaseDescriptorSets(s_Data2D.QuadDescriptorSets.data() + descriptorSetsToFree,
                                                                      descriptorSetsToFree);
        }
    }

    s_Data2D.QuadVertexBufferPtr          = s_Data2D.QuadVertexBufferBase;
    s_Data2D.CurrentQuadVertexBufferIndex = 0;

    s_Data2D.CurrentDescriptorSetIndex = 0;
    s_Data2D.CurrentTextureSlotIndex   = 1;

    s_Data2D.QuadIndexCount        = 0;
    Renderer::GetStats().QuadCount = 0;

    for (auto& StagingBuffer : s_Data2D.StagingStorage.StagingBuffers)
    {
        StagingBuffer->Size = 0;
    }
    s_Data2D.StagingStorage.CurrentStagingBufferIndex = 0;
}

void VulkanRenderer2D::DrawQuadImpl(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
{
    DrawRotatedQuadImpl(position, size, glm::vec3(0.0f), color);
}

void VulkanRenderer2D::DrawQuadImpl(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation,
                                    const Ref<Texture2D>& texture, const glm::vec4& color)
{
    if (s_Data2D.QuadIndexCount >= s_Data2D.MaxIndices)
    {
        FlushAndReset();
    }

    float TextureId = 0.0f;
    for (uint32_t i = 1; i <= s_Data2D.CurrentTextureSlotIndex; ++i)
    {
        if (s_Data2D.TextureSlots[i] == texture)
        {
            TextureId = (float)i;
            break;
        }
    }

    if (TextureId == 0.0f)
    {
        if (s_Data2D.CurrentTextureSlotIndex + 1 >= VulkanRenderer2DStorage::MaxTextureSlots)
        {
            FlushAndReset();
        }

        const auto VulkanTexture                                = std::static_pointer_cast<VulkanTexture2D>(texture);
        s_Data2D.TextureSlots[s_Data2D.CurrentTextureSlotIndex] = VulkanTexture;
        TextureId                                               = (float)s_Data2D.CurrentTextureSlotIndex;
        ++s_Data2D.CurrentTextureSlotIndex;
    }

    const auto Transform = glm::translate(glm::mat4(1.0f), position) *
                           glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0, 0, 1)) *
                           glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
    DrawQuadInternal(Transform, color, TextureId);
}

void VulkanRenderer2D::DrawRotatedQuadImpl(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec3& InRotation,
                                           const glm::vec4& InColor)
{
    const auto Transform = glm::translate(glm::mat4(1.0f), InPosition) *
                           glm::rotate(glm::mat4(1.0f), glm::radians(InRotation.z), glm::vec3(0, 0, 1)) *
                           glm::scale(glm::mat4(1.0f), {InSize.x, InSize.y, 1.0f});

    DrawQuadImpl(Transform, InColor);
}

void VulkanRenderer2D::DrawQuadImpl(const glm::mat4& transform, const glm::vec4& color)
{
    if (s_Data2D.QuadIndexCount >= s_Data2D.MaxIndices)
    {
        FlushAndReset();
    }

    const float TextureId = 0.0f;  // White by default
    DrawQuadInternal(transform, color, TextureId);
}

void VulkanRenderer2D::DrawTexturedQuadImpl(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture,
                                            const glm::vec4& blendColor)
{
    if (s_Data2D.QuadIndexCount >= s_Data2D.MaxIndices)
    {
        FlushAndReset();
    }

    float TextureId = 0.0f;
    for (uint32_t i = 1; i <= s_Data2D.CurrentTextureSlotIndex; ++i)
    {
        if (s_Data2D.TextureSlots[i] == texture)
        {
            TextureId = (float)i;
            break;
        }
    }

    if (TextureId == 0.0f)
    {
        if (s_Data2D.CurrentTextureSlotIndex + 1 >= VulkanRenderer2DStorage::MaxTextureSlots)
        {
            FlushAndReset();
        }

        const auto VulkanTexture                                = std::static_pointer_cast<VulkanTexture2D>(texture);
        s_Data2D.TextureSlots[s_Data2D.CurrentTextureSlotIndex] = VulkanTexture;
        TextureId                                               = (float)s_Data2D.CurrentTextureSlotIndex;
        ++s_Data2D.CurrentTextureSlotIndex;
    }

    const auto Transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
    DrawQuadInternal(Transform, blendColor, TextureId);
}

void VulkanRenderer2D::DrawQuadInternal(const glm::mat4& transform, const glm::vec4& blendColor, const float textureID)
{
    for (uint32_t i = 0; i < 4; ++i)
    {
        s_Data2D.QuadVertexBufferPtr->Position  = transform * s_Data2D.QuadVertexPositions[i];
        s_Data2D.QuadVertexBufferPtr->Color     = blendColor;
        s_Data2D.QuadVertexBufferPtr->TexCoord  = VulkanRenderer2DStorage::TextureCoords[i];
        s_Data2D.QuadVertexBufferPtr->TextureId = textureID;
        ++s_Data2D.QuadVertexBufferPtr;
    }

    s_Data2D.QuadIndexCount += 6;

    s_Data2D.PushConstants.TransformMatrix = s_Data2D.CameraProjectionMatrix;
    s_Data2D.PushConstants.Color           = blendColor;

    ++Renderer::GetStats().QuadCount;
}

void VulkanRenderer2D::FlushAndReset()
{
    FlushImpl();

    for (uint32_t i = 1; i <= s_Data2D.CurrentTextureSlotIndex; ++i)
    {
        s_Data2D.TextureSlots[i] = nullptr;
    }

    if (s_Data2D.CurrentTextureSlotIndex > 1)
    {
        auto descriptorInfo = s_Data2D.TextureSlots[0]->GetImageDescriptorInfo();
        const auto TextureWriteSet =
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
                                           s_Data2D.QuadDescriptorSets[s_Data2D.CurrentDescriptorSetIndex].Handle, 1, &descriptorInfo);
        vkUpdateDescriptorSets(m_Context.GetDevice()->GetLogicalDevice(), 1, &TextureWriteSet, 0, nullptr);
    }

    s_Data2D.CurrentTextureSlotIndex = 1;
    s_Data2D.QuadIndexCount          = 0;
    s_Data2D.QuadVertexBufferPtr     = s_Data2D.QuadVertexBufferBase;

    // Reset
    s_Data2D.StagingStorage.CurrentStagingBufferIndex = 0;
    for (auto& StagingBuffer : s_Data2D.StagingStorage.StagingBuffers)
    {
        StagingBuffer->Size = 0;
    }
}

void VulkanRenderer2D::PreallocateRenderStorage()
{
    if (s_Data2D.CurrentDescriptorSetIndex >= s_Data2D.QuadDescriptorSets.size())
    {
        DescriptorSet QuadSamplerDescriptorSet;
        GNT_ASSERT(m_Context.GetDescriptorAllocator()->Allocate(QuadSamplerDescriptorSet, s_Data2D.QuadDescriptorSetLayout),
                   "Failed to allocate descriptor sets!");
        s_Data2D.QuadDescriptorSets.push_back(QuadSamplerDescriptorSet);
    }

    if (s_Data2D.CurrentQuadVertexBufferIndex >= s_Data2D.QuadVertexBuffers.size())
    {
        BufferInfo VertexBufferInfo = {};
        VertexBufferInfo.Usage      = EBufferUsageFlags::VERTEX_BUFFER;
        VertexBufferInfo.Layout     = s_Data2D.VertexBufferLayout;
        s_Data2D.QuadVertexBuffers.emplace_back(new VulkanVertexBuffer(VertexBufferInfo));
    }
}

void VulkanRenderer2D::FlushImpl()
{
    if (s_Data2D.QuadIndexCount <= 0) return;

    PreallocateRenderStorage();

    const uint32_t DataSize = (uint32_t)((uint8_t*)s_Data2D.QuadVertexBufferPtr - (uint8_t*)s_Data2D.QuadVertexBufferBase);

    // Check if we're out of staging buffer memory bounds or not
    if (s_Data2D.StagingStorage.StagingBuffers[s_Data2D.StagingStorage.CurrentStagingBufferIndex]->Size + DataSize >
        s_Data2D.StagingStorage.StagingBuffers[s_Data2D.StagingStorage.CurrentStagingBufferIndex]->Capacity)
    {
        // Get new staging buffer && set capacity
        ++Renderer::GetStats().StagingVertexBuffers;

        Scoped<VulkanStagingStorage::StagingBuffer> QuadVertexStagingBuffer(new VulkanStagingStorage::StagingBuffer());
        BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, s_Data2D.DefaultVertexBufferSize, (*QuadVertexStagingBuffer).Buffer);
        QuadVertexStagingBuffer->Capacity = s_Data2D.DefaultVertexBufferSize;
        s_Data2D.StagingStorage.StagingBuffers.push_back(std::move(QuadVertexStagingBuffer));
        ++s_Data2D.StagingStorage.CurrentStagingBufferIndex;
    }
    BufferUtils::CopyDataToBuffer(s_Data2D.StagingStorage.StagingBuffers[s_Data2D.StagingStorage.CurrentStagingBufferIndex]->Buffer,
                                  DataSize, s_Data2D.QuadVertexBufferBase);
    s_Data2D.StagingStorage.StagingBuffers[s_Data2D.StagingStorage.CurrentStagingBufferIndex]->Size += DataSize;

    s_Data2D.QuadVertexBuffers[s_Data2D.CurrentQuadVertexBufferIndex]->SetStagedData(
        (*s_Data2D.StagingStorage.StagingBuffers[s_Data2D.StagingStorage.CurrentStagingBufferIndex]).Buffer, DataSize);

    if (s_Data2D.CurrentTextureSlotIndex > 1)
    {
        std::vector<VkDescriptorImageInfo> ImageInfos;
        for (uint32_t i = 0; i < s_Data2D.CurrentTextureSlotIndex; ++i)
        {
            ImageInfos.emplace_back(s_Data2D.TextureSlots[i]->GetImageDescriptorInfo());
        }
        const auto TextureWriteSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
                                                                    s_Data2D.QuadDescriptorSets[s_Data2D.CurrentDescriptorSetIndex].Handle,
                                                                    (uint32_t)ImageInfos.size(), ImageInfos.data());
        vkUpdateDescriptorSets(m_Context.GetDevice()->GetLogicalDevice(), 1, &TextureWriteSet, 0, nullptr);
    }

    auto& GeneralStorageData = VulkanRenderer::GetStorageData();
    GeneralStorageData.CurrentCommandBuffer->BeginDebugLabel("2D-Batch", glm::vec4(0.5f, 0.0f, 0.0f, 1.0f));
    GeneralStorageData.GeometryFramebuffer->BeginRenderPass(GeneralStorageData.CurrentCommandBuffer->Get());

    GeneralStorageData.CurrentCommandBuffer->BindPipeline(s_Data2D.QuadPipeline);

    VkDeviceSize Offset = 0;
    GeneralStorageData.CurrentCommandBuffer->BindVertexBuffers(
        0, 1, &s_Data2D.QuadVertexBuffers[s_Data2D.CurrentQuadVertexBufferIndex]->Get(), &Offset);
    GeneralStorageData.CurrentCommandBuffer->BindIndexBuffer(s_Data2D.QuadIndexBuffer->Get(), 0, VK_INDEX_TYPE_UINT32);

    GeneralStorageData.CurrentCommandBuffer->BindDescriptorSets(s_Data2D.QuadPipeline->GetLayout(), 0, 1,
                                                                &s_Data2D.QuadDescriptorSets[s_Data2D.CurrentDescriptorSetIndex].Handle);
    GeneralStorageData.CurrentCommandBuffer->BindPushConstants(s_Data2D.QuadPipeline->GetLayout(),
                                                               s_Data2D.QuadPipeline->GetPushConstantsShaderStageFlags(), 0,
                                                               s_Data2D.QuadPipeline->GetPushConstantsSize(), &s_Data2D.PushConstants);

    GeneralStorageData.CurrentCommandBuffer->DrawIndexed(s_Data2D.QuadIndexCount);

    GeneralStorageData.GeometryFramebuffer->EndRenderPass(GeneralStorageData.CurrentCommandBuffer->Get());
    GeneralStorageData.CurrentCommandBuffer->EndDebugLabel();

    ++Renderer::GetStats().DrawCalls;
    ++s_Data2D.CurrentDescriptorSetIndex;
    ++s_Data2D.CurrentQuadVertexBufferIndex;
}

void VulkanRenderer2D::Destroy()
{
    m_Context.GetDevice()->WaitDeviceOnFinish();

    vkDestroyDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), s_Data2D.QuadDescriptorSetLayout, nullptr);
    m_Context.GetDescriptorAllocator()->ReleaseDescriptorSets(s_Data2D.QuadDescriptorSets.data(), s_Data2D.QuadDescriptorSets.size());

    delete[] s_Data2D.QuadVertexBufferBase;
    s_Data2D.QuadPipeline->Destroy();
    s_Data2D.QuadIndexBuffer->Destroy();

    for (auto& QuadVertexBuffer : s_Data2D.QuadVertexBuffers)
        QuadVertexBuffer->Destroy();

    for (auto& QuadVertexStagingBuffer : s_Data2D.StagingStorage.StagingBuffers)
        BufferUtils::DestroyBuffer((*QuadVertexStagingBuffer).Buffer);
}

}  // namespace Gauntlet