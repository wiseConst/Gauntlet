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
    s_Data2D.QuadVertexBufferBase = new QuadVertex[s_Data2D.MaxVertices];

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

    s_Data2D.TextureSlots[0] = std::static_pointer_cast<VulkanTexture2D>(Renderer::GetStorageData().WhiteTexture);

    // Default 2D graphics pipeline creation
    {
        auto FlatColorShader        = std::static_pointer_cast<VulkanShader>(ShaderLibrary::Load("Resources/Cached/Shaders/FlatColor"));
        s_Data2D.VertexBufferLayout = FlatColorShader->GetVertexBufferLayout();
        s_Data2D.QuadDescriptorSetLayout = FlatColorShader->GetDescriptorSetLayouts()[0];

        PipelineSpecification PipelineSpec = {};
        PipelineSpec.Name                  = "Quad2D";
        PipelineSpec.bDepthTest            = VK_TRUE;
        PipelineSpec.bDepthWrite           = VK_TRUE;
        PipelineSpec.DepthCompareOp        = ECompareOp::COMPARE_OP_LESS;
        PipelineSpec.TargetFramebuffer     = VulkanRenderer::GetVulkanStorageData().GeometryFramebuffer;
        PipelineSpec.Shader                = FlatColorShader;
        PipelineSpec.bDynamicPolygonMode   = VK_TRUE;

        s_Data2D.QuadPipeline = MakeRef<VulkanPipeline>(PipelineSpec);
    }

    s_Data2D.QuadStagingBuffer = MakeScoped<VulkanRenderer2DStorage::StagingBuffer>();
    BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, s_Data2D.DefaultVertexBufferSize, s_Data2D.QuadStagingBuffer->Buffer);
    s_Data2D.QuadStagingBuffer->Capacity = s_Data2D.DefaultVertexBufferSize;

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
            const uint64_t descriptorSetsToFree =
                s_Data2D.QuadDescriptorSets.size() - static_cast<uint64_t>(s_Data2D.CurrentDescriptorSetIndex);
            m_Context.GetDescriptorAllocator()->ReleaseDescriptorSets(s_Data2D.QuadDescriptorSets.data() + descriptorSetsToFree,
                                                                      static_cast<uint32_t>(descriptorSetsToFree));
        }
    }

    s_Data2D.QuadVertexBufferPtr          = s_Data2D.QuadVertexBufferBase;
    s_Data2D.CurrentQuadVertexBufferIndex = 0;

    s_Data2D.CurrentDescriptorSetIndex = 0;
    s_Data2D.CurrentTextureSlotIndex   = 1;

    s_Data2D.QuadIndexCount        = 0;
    Renderer::GetStats().QuadCount = 0;
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
    const auto transform = glm::translate(glm::mat4(1.0f), InPosition) *
                           glm::rotate(glm::mat4(1.0f), glm::radians(InRotation.z), glm::vec3(0, 0, 1)) *
                           glm::scale(glm::mat4(1.0f), {InSize.x, InSize.y, 1.0f});

    DrawQuadImpl(transform, InColor);
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

void VulkanRenderer2D::DrawTexturedQuadImpl(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation,
                                            const Ref<Texture2D>& textureAtlas, const glm::vec2& spriteCoords, const glm::vec2& spriteSize)
{
    if (s_Data2D.QuadIndexCount >= s_Data2D.MaxIndices)
    {
        FlushAndReset();
    }

    float textureID = 0.0f;
    for (uint32_t i = 1; i <= s_Data2D.CurrentTextureSlotIndex; ++i)
    {
        if (s_Data2D.TextureSlots[i] == textureAtlas)
        {
            textureID = (float)i;
            break;
        }
    }

    if (textureID == 0.0f)
    {
        if (s_Data2D.CurrentTextureSlotIndex + 1 >= VulkanRenderer2DStorage::MaxTextureSlots)
        {
            FlushAndReset();
        }

        const auto VulkanTexture                                = std::static_pointer_cast<VulkanTexture2D>(textureAtlas);
        s_Data2D.TextureSlots[s_Data2D.CurrentTextureSlotIndex] = VulkanTexture;
        textureID                                               = (float)s_Data2D.CurrentTextureSlotIndex;
        ++s_Data2D.CurrentTextureSlotIndex;
    }

    const glm::vec2 spriteSheetSize = glm::vec2(textureAtlas->GetWidth(), textureAtlas->GetHeight());
    const glm::vec2 texCoords[]     = {
        {(spriteCoords.x * spriteSize.x) / spriteSheetSize.x, (spriteCoords.y * spriteSize.y) / spriteSheetSize.y},              //
        {((spriteCoords.x + 1) * spriteSize.x) / spriteSheetSize.x, (spriteCoords.y * spriteSize.y) / spriteSheetSize.y},        //
        {((spriteCoords.x + 1) * spriteSize.x) / spriteSheetSize.x, ((spriteCoords.y + 1) * spriteSize.y) / spriteSheetSize.y},  //
        {spriteCoords.x * spriteSize.x / spriteSheetSize.x, ((spriteCoords.y + 1) * spriteSize.y) / spriteSheetSize.y}           //
    };

    const glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1, 0, 0)) *  // rotation around X
                                     glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0, 1, 0)) *  // around Y
                                     glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0, 0, 1));   // around Z
    const glm::mat4 transform =
        glm::translate(glm::mat4(1.0f), position) * rotationMatrix * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
    for (uint32_t i = 0; i < 4; ++i)
    {
        s_Data2D.QuadVertexBufferPtr->Position  = transform * s_Data2D.QuadVertexPositions[i];
        s_Data2D.QuadVertexBufferPtr->Color     = glm::vec4(1.0f);
        s_Data2D.QuadVertexBufferPtr->TexCoord  = texCoords[i];
        s_Data2D.QuadVertexBufferPtr->TextureId = textureID;
        ++s_Data2D.QuadVertexBufferPtr;
    }

    s_Data2D.QuadIndexCount += 6;
    s_Data2D.PushConstants.TransformMatrix = s_Data2D.CameraProjectionMatrix;
    ++Renderer::GetStats().QuadCount;
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
    s_Data2D.PushConstants.Data            = blendColor;

    ++Renderer::GetStats().QuadCount;
}

void VulkanRenderer2D::FlushAndReset()
{
    FlushImpl();

    for (uint32_t i = 1; i <= s_Data2D.CurrentTextureSlotIndex; ++i)
    {
        s_Data2D.TextureSlots[i] = nullptr;
    }

    s_Data2D.CurrentTextureSlotIndex = 1;
    s_Data2D.QuadIndexCount          = 0;
    s_Data2D.QuadVertexBufferPtr     = s_Data2D.QuadVertexBufferBase;
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

    // Check if dataSize we want to upload onto staging buffer is greater than capacity, then resize.
    if (DataSize > s_Data2D.QuadStagingBuffer->Capacity)
    {
        BufferUtils::DestroyBuffer(s_Data2D.QuadStagingBuffer->Buffer);
        BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, DataSize, s_Data2D.QuadStagingBuffer->Buffer);
        s_Data2D.QuadStagingBuffer->Capacity = DataSize;
    }
    BufferUtils::CopyDataToBuffer(s_Data2D.QuadStagingBuffer->Buffer, DataSize, s_Data2D.QuadVertexBufferBase);

    s_Data2D.QuadVertexBuffers[s_Data2D.CurrentQuadVertexBufferIndex]->SetStagedData(s_Data2D.QuadStagingBuffer->Buffer, DataSize);

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

    auto& GeneralStorageData = VulkanRenderer::GetVulkanStorageData();
    GeneralStorageData.CurrentCommandBuffer->BeginDebugLabel("2D-Batch", glm::vec4(0.5f, 0.0f, 0.0f, 1.0f));
    GeneralStorageData.GeometryFramebuffer->BeginRenderPass(GeneralStorageData.CurrentCommandBuffer->Get());

    GeneralStorageData.CurrentCommandBuffer->SetPipelinePolygonMode(
        s_Data2D.QuadPipeline, Renderer::GetSettings().ShowWireframes ? EPolygonMode::POLYGON_MODE_LINE : EPolygonMode::POLYGON_MODE_FILL);
    GeneralStorageData.CurrentCommandBuffer->BindPipeline(s_Data2D.QuadPipeline);

    VkDeviceSize Offset = 0;
    VkBuffer vb         = (VkBuffer)s_Data2D.QuadVertexBuffers[s_Data2D.CurrentQuadVertexBufferIndex]->Get();
    GeneralStorageData.CurrentCommandBuffer->BindVertexBuffers(0, 1, &vb, &Offset);

    VkBuffer ib = (VkBuffer)s_Data2D.QuadIndexBuffer->Get();
    GeneralStorageData.CurrentCommandBuffer->BindIndexBuffer(ib);

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

    m_Context.GetDescriptorAllocator()->ReleaseDescriptorSets(s_Data2D.QuadDescriptorSets.data(),
                                                              static_cast<uint32_t>(s_Data2D.QuadDescriptorSets.size()));

    delete[] s_Data2D.QuadVertexBufferBase;
    s_Data2D.QuadPipeline->Destroy();
    s_Data2D.QuadIndexBuffer->Destroy();

    for (auto& QuadVertexBuffer : s_Data2D.QuadVertexBuffers)
        QuadVertexBuffer->Destroy();

    BufferUtils::DestroyBuffer(s_Data2D.QuadStagingBuffer->Buffer);
}

}  // namespace Gauntlet