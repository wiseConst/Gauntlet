#include "GauntletPCH.h"
#include "VulkanRenderer2D.h"

#include "VulkanContext.h"
#include "VulkanUtility.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanShader.h"
#include "VulkanPipeline.h"

#include "VulkanBuffer.h"
#include "VulkanTexture.h"
#include "VulkanCommandPool.h"
#include "VulkanDescriptors.h"
#include "VulkanFramebuffer.h"
#include "VulkanRenderer.h"

namespace Gauntlet
{
// The way it works now: create staging buffer && vertex buffer every frame and destroy

// The way it should  work: Allocate one big staging buffer and copy it's data to vertex buffer, that's it.
struct VulkanStagingStorage
{
    struct StagingBuffer
    {
        AllocatedBuffer Buffer;
        size_t Capacity = 0;
        size_t Size     = 0;
    };

    std::vector<Scoped<StagingBuffer>> StagingBuffers;
    uint32_t CurrentStagingBufferIndex = 0;
};

struct VulkanRenderer2DStorage
{
    static constexpr uint32_t MaxQuads          = 2500;
    static constexpr uint32_t MaxVertices       = MaxQuads * 4;
    static constexpr uint32_t MaxIndices        = MaxQuads * 6;
    static constexpr uint32_t MaxTextureSlots   = 32;
    static constexpr glm::vec2 TextureCoords[4] = {glm::vec2(0.0f, 0.0f),  //
                                                   glm::vec2(1.0f, 0.0f),  //
                                                   glm::vec2(1.0f, 1.0f),  //
                                                   glm::vec2(0.0f, 1.0f)};

    static constexpr glm::vec4 QuadVertexPositions[4] = {{-0.5f, -0.5f, 0.0f, 1.0f},  //
                                                         {0.5f, -0.5f, 0.0f, 1.0f},   //
                                                         {0.5f, 0.5f, 0.0f, 1.0f},    //
                                                         {-0.5f, 0.5f, 0.0f, 1.0f}};

    static constexpr size_t DefaultVertexBufferSize = MaxVertices * sizeof(QuadVertex);

    VulkanStagingStorage StagingStorage;
    std::vector<Ref<VulkanVertexBuffer>> QuadVertexBuffers;
    uint32_t CurrentQuadVertexBufferIndex = 0;

    BufferLayout VertexBufferLayout;
    QuadVertex* QuadVertexBufferBase = nullptr;
    QuadVertex* QuadVertexBufferPtr  = nullptr;

    Ref<VulkanIndexBuffer> QuadIndexBuffer;
    uint32_t QuadIndexCount = 0;

    Ref<VulkanPipeline> QuadPipeline;
    Ref<VulkanShader> VertexShader;
    Ref<VulkanShader> FragmentShader;

    VkDescriptorSetLayout QuadDescriptorSetLayout;
    std::vector<VkDescriptorSet> QuadDescriptorSets;
    uint32_t CurrentDescriptorSetIndex = 0;

    std::array<Ref<VulkanTexture2D>, MaxTextureSlots> TextureSlots;
    Ref<VulkanTexture2D> QuadWhiteTexture;  // Texture Slot 0 = white tex
    uint32_t CurrentTextureSlotIndex = 1;   // 0 slot already tied with white texture

    glm::mat4 CameraProjectionMatrix = glm::mat4(1.0f);
    MeshPushConstants PushConstants;
};

static VulkanRenderer2DStorage s_Data;

VulkanRenderer2D::VulkanRenderer2D() : m_Context((VulkanContext&)VulkanContext::Get())
{
    Create();
}

void VulkanRenderer2D::Create()
{
    // Vertex buffer creation
    {
        s_Data.VertexBufferLayout = {
            {EShaderDataType::Vec3, "InPostion"},    //
            {EShaderDataType::Vec3, "InNormal"},     //
            {EShaderDataType::Vec4, "InColor"},      //
            {EShaderDataType::Vec2, "InTexCoord"},   //
            {EShaderDataType::FLOAT, "InTextureId"}  //
        };                                           //

        s_Data.QuadVertexBufferBase = new QuadVertex[s_Data.MaxVertices];
    }

    // Index Buffer creation
    {
        uint32_t* QuadIndices = new uint32_t[s_Data.MaxIndices];

        uint32_t Offset = 0;
        for (uint32_t i = 0; i < s_Data.MaxIndices; i += 6)
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
        IndexBufferInfo.Count      = s_Data.MaxIndices;
        IndexBufferInfo.Size       = s_Data.MaxIndices * sizeof(uint32_t);
        IndexBufferInfo.Data       = QuadIndices;
        s_Data.QuadIndexBuffer.reset(new VulkanIndexBuffer(IndexBufferInfo));

        delete[] QuadIndices;
    }

    // Default white texture
    uint32_t WhiteTextureData = 0xffffffff;
    s_Data.QuadWhiteTexture.reset(new VulkanTexture2D(&WhiteTextureData, sizeof(uint32_t), 1, 1));

    s_Data.TextureSlots[0] = s_Data.QuadWhiteTexture;

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
                                         &s_Data.QuadDescriptorSetLayout),
             "Failed to create descriptor set layout!");

    // Default 2D graphics pipeline creation
    {
        PipelineSpecification PipelineSpec = {};
        PipelineSpec.TargetFramebuffer     = VulkanRenderer::GetPostProcessFramebuffer() /*m_Context.GetMainRenderPass()*/;
        PipelineSpec.bDepthTest            = true;
        PipelineSpec.bDepthWrite           = true;
        PipelineSpec.DepthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;

        const std::vector<VkVertexInputBindingDescription> ShaderBindingDescriptions = {
            Utility::GetShaderBindingDescription(0, s_Data.VertexBufferLayout.GetStride())};
        PipelineSpec.ShaderBindingDescriptions = ShaderBindingDescriptions;

        std::vector<VkVertexInputAttributeDescription> ShaderAttributeDescriptions(s_Data.VertexBufferLayout.GetElements().size());
        for (uint32_t i = 0; i < ShaderAttributeDescriptions.size(); ++i)
        {
            ShaderAttributeDescriptions[i] = Utility::GetShaderAttributeDescription(
                0, Utility::GauntletShaderDataTypeToVulkan(s_Data.VertexBufferLayout.GetElements()[i].Type), i,
                (uint32_t)s_Data.VertexBufferLayout.GetElements()[i].Offset);
        }

        PipelineSpec.ShaderAttributeDescriptions = ShaderAttributeDescriptions;

        s_Data.VertexShader   = Ref<VulkanShader>(new VulkanShader(std::string(ASSETS_PATH) + "Shaders/FlatColor.vert.spv"));
        s_Data.FragmentShader = Ref<VulkanShader>(new VulkanShader(std::string(ASSETS_PATH) + "Shaders/FlatColor.frag.spv"));

        std::vector<PipelineSpecification::ShaderStage> ShaderStages(2);
        ShaderStages[0].Stage  = EShaderStage::SHADER_STAGE_VERTEX;
        ShaderStages[0].Shader = s_Data.VertexShader;

        ShaderStages[1].Stage  = EShaderStage::SHADER_STAGE_FRAGMENT;
        ShaderStages[1].Shader = s_Data.FragmentShader;

        PipelineSpec.ShaderStages       = ShaderStages;
        PipelineSpec.PushConstantRanges = {Utility::GetPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(MeshPushConstants))};

        PipelineSpec.DescriptorSetLayouts = {s_Data.QuadDescriptorSetLayout};

        s_Data.QuadPipeline.reset(new VulkanPipeline(PipelineSpec));
    }

    m_Context.AddSwapchainResizeCallback([this] { s_Data.QuadPipeline->Invalidate(); });

    // Initially 1 staging buffer
    Scoped<VulkanStagingStorage::StagingBuffer> QuadVertexStagingBuffer(new VulkanStagingStorage::StagingBuffer());
    BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, s_Data.DefaultVertexBufferSize, (*QuadVertexStagingBuffer).Buffer);
    QuadVertexStagingBuffer->Capacity = s_Data.DefaultVertexBufferSize;
    s_Data.StagingStorage.StagingBuffers.push_back(std::move(QuadVertexStagingBuffer));
    ++Renderer::GetStats().StagingVertexBuffers;
}

void VulkanRenderer2D::BeginImpl()
{
    s_Data.QuadVertexBufferPtr          = s_Data.QuadVertexBufferBase;
    s_Data.CurrentQuadVertexBufferIndex = 0;

    s_Data.CurrentDescriptorSetIndex = 0;
    s_Data.CurrentTextureSlotIndex   = 1;

    s_Data.QuadIndexCount          = 0;
    Renderer::GetStats().QuadCount = 0;

    for (auto& StagingBuffer : s_Data.StagingStorage.StagingBuffers)
    {
        StagingBuffer->Size = 0;
    }
    s_Data.StagingStorage.CurrentStagingBufferIndex = 0;
}

void VulkanRenderer2D::BeginSceneImpl(const Camera& InCamera)
{
    s_Data.CameraProjectionMatrix = InCamera.GetViewProjectionMatrix();
}

void VulkanRenderer2D::DrawQuadImpl(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec4& InColor)
{
    DrawRotatedQuadImpl(InPosition, InSize, glm::vec3(0.0f), InColor);
}

void VulkanRenderer2D::DrawQuadImpl(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec3& InRotation,
                                    const Ref<Texture2D>& InTexture, const glm::vec4& InColor)
{
    if (s_Data.QuadIndexCount >= s_Data.MaxIndices)
    {
        FlushAndReset();
    }

    float TextureId = 0.0f;
    for (uint32_t i = 0; i < s_Data.CurrentTextureSlotIndex; ++i)
    {
        if (s_Data.TextureSlots[i] == InTexture)
        {
            TextureId = (float)i;
            break;
        }
    }

    if (TextureId == 0.0f)
    {
        if (s_Data.CurrentTextureSlotIndex + 1 >= VulkanRenderer2DStorage::MaxTextureSlots)
        {
            FlushAndReset();
        }

        const auto VulkanTexture                            = std::static_pointer_cast<VulkanTexture2D>(InTexture);
        s_Data.TextureSlots[s_Data.CurrentTextureSlotIndex] = VulkanTexture;
        TextureId                                           = (float)s_Data.CurrentTextureSlotIndex;
        ++s_Data.CurrentTextureSlotIndex;
    }

    const auto TransformMatrix = glm::translate(glm::mat4(1.0f), InPosition) *
                                 glm::rotate(glm::mat4(1.0f), glm::radians(InRotation.z), glm::vec3(0, 0, 1)) *
                                 glm::scale(glm::mat4(1.0f), {InSize.x, InSize.y, 1.0f});

    for (uint32_t i = 0; i < 4; ++i)
    {
        s_Data.QuadVertexBufferPtr->Position  = TransformMatrix * s_Data.QuadVertexPositions[i];
        s_Data.QuadVertexBufferPtr->Color     = InColor;
        s_Data.QuadVertexBufferPtr->TexCoord  = VulkanRenderer2DStorage::TextureCoords[i];
        s_Data.QuadVertexBufferPtr->TextureId = TextureId;
        ++s_Data.QuadVertexBufferPtr;
    }

    s_Data.QuadIndexCount += 6;

    s_Data.PushConstants.TransformMatrix = s_Data.CameraProjectionMatrix;
    s_Data.PushConstants.Color           = InColor;

    ++Renderer::GetStats().QuadCount;
}

void VulkanRenderer2D::DrawRotatedQuadImpl(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec3& InRotation,
                                           const glm::vec4& InColor)
{
    if (s_Data.QuadIndexCount >= s_Data.MaxIndices)
    {
        FlushAndReset();
    }

    const float TextureId = 0.0f;  // White by default

    const auto TransformMatrix = glm::translate(glm::mat4(1.0f), InPosition) *
                                 glm::rotate(glm::mat4(1.0f), glm::radians(InRotation.z), glm::vec3(0, 0, 1)) *
                                 glm::scale(glm::mat4(1.0f), {InSize.x, InSize.y, 1.0f});

    for (uint32_t i = 0; i < 4; ++i)
    {
        s_Data.QuadVertexBufferPtr->Position  = TransformMatrix * s_Data.QuadVertexPositions[i];
        s_Data.QuadVertexBufferPtr->Color     = InColor;
        s_Data.QuadVertexBufferPtr->TexCoord  = VulkanRenderer2DStorage::TextureCoords[i];
        s_Data.QuadVertexBufferPtr->TextureId = TextureId;
        ++s_Data.QuadVertexBufferPtr;
    }

    s_Data.QuadIndexCount += 6;

    s_Data.PushConstants.TransformMatrix = s_Data.CameraProjectionMatrix;
    s_Data.PushConstants.Color           = InColor;

    ++Renderer::GetStats().QuadCount;
}

void VulkanRenderer2D::DrawTexturedQuadImpl(const glm::vec3& InPosition, const glm::vec2& InSize, const Ref<Texture2D>& InTexture,
                                            const glm::vec4& InBlendColor)
{
    if (s_Data.QuadIndexCount >= s_Data.MaxIndices)
    {
        FlushAndReset();
    }

    float TextureId = 0.0f;
    for (uint32_t i = 0; i < s_Data.CurrentTextureSlotIndex; ++i)
    {
        if (s_Data.TextureSlots[i] == InTexture)
        {
            TextureId = (float)i;
            break;
        }
    }

    if (TextureId == 0.0f)
    {
        if (s_Data.CurrentTextureSlotIndex + 1 >= VulkanRenderer2DStorage::MaxTextureSlots)
        {
            FlushAndReset();
        }

        const auto VulkanTexture                            = std::static_pointer_cast<VulkanTexture2D>(InTexture);
        s_Data.TextureSlots[s_Data.CurrentTextureSlotIndex] = VulkanTexture;
        TextureId                                           = (float)s_Data.CurrentTextureSlotIndex;
        ++s_Data.CurrentTextureSlotIndex;
    }

    const auto TransformMatrix = glm::translate(glm::mat4(1.0f), InPosition) * glm::scale(glm::mat4(1.0f), {InSize.x, InSize.y, 1.0f});

    for (uint32_t i = 0; i < 4; ++i)
    {
        s_Data.QuadVertexBufferPtr->Position  = TransformMatrix * s_Data.QuadVertexPositions[i];
        s_Data.QuadVertexBufferPtr->Color     = InBlendColor;
        s_Data.QuadVertexBufferPtr->TexCoord  = VulkanRenderer2DStorage::TextureCoords[i];
        s_Data.QuadVertexBufferPtr->TextureId = TextureId;
        ++s_Data.QuadVertexBufferPtr;
    }

    s_Data.QuadIndexCount += 6;

    s_Data.PushConstants.TransformMatrix = s_Data.CameraProjectionMatrix;
    s_Data.PushConstants.Color           = InBlendColor;

    ++Renderer::GetStats().QuadCount;
}

void VulkanRenderer2D::FlushAndReset()
{
    FlushImpl();

    for (uint32_t i = 1; i < s_Data.TextureSlots.size(); ++i)
    {
        s_Data.TextureSlots[i] = nullptr;  // Potential memory leak?
    }

    s_Data.CurrentTextureSlotIndex = 1;
    s_Data.QuadIndexCount          = 0;
    s_Data.QuadVertexBufferPtr     = s_Data.QuadVertexBufferBase;

    // Reset
    s_Data.StagingStorage.CurrentStagingBufferIndex = 0;
    for (auto& StagingBuffer : s_Data.StagingStorage.StagingBuffers)
    {
        StagingBuffer->Size = 0;
    }
}

void VulkanRenderer2D::PreallocateRenderStorage()
{
    if (s_Data.CurrentDescriptorSetIndex >= s_Data.QuadDescriptorSets.size())
    {
        VkDescriptorSet QuadSamplerDescriptorSet = VK_NULL_HANDLE;
        GNT_ASSERT(m_Context.GetDescriptorAllocator()->Allocate(&QuadSamplerDescriptorSet, s_Data.QuadDescriptorSetLayout),
                   "Failed to allocate descriptor sets!");
        s_Data.QuadDescriptorSets.push_back(QuadSamplerDescriptorSet);
    }

    if (s_Data.CurrentQuadVertexBufferIndex >= s_Data.QuadVertexBuffers.size())
    {
        BufferInfo VertexBufferInfo = {};
        VertexBufferInfo.Usage      = EBufferUsageFlags::VERTEX_BUFFER;
        VertexBufferInfo.Layout     = s_Data.VertexBufferLayout;
        s_Data.QuadVertexBuffers.emplace_back(new VulkanVertexBuffer(VertexBufferInfo));
    }
}

void VulkanRenderer2D::FlushImpl()
{
    if (s_Data.QuadIndexCount <= 0) return;

    PreallocateRenderStorage();

    const uint32_t DataSize = (uint32_t)((uint8_t*)s_Data.QuadVertexBufferPtr - (uint8_t*)s_Data.QuadVertexBufferBase);

    // Check if we're out of staging buffer memory bounds or not
    if (s_Data.StagingStorage.StagingBuffers[s_Data.StagingStorage.CurrentStagingBufferIndex]->Size + DataSize >
        s_Data.StagingStorage.StagingBuffers[s_Data.StagingStorage.CurrentStagingBufferIndex]->Capacity)
    {
        // Get new staging buffer && set capacity
        ++Renderer::GetStats().StagingVertexBuffers;

        Scoped<VulkanStagingStorage::StagingBuffer> QuadVertexStagingBuffer(new VulkanStagingStorage::StagingBuffer());
        BufferUtils::CreateBuffer(EBufferUsageFlags::STAGING_BUFFER, s_Data.DefaultVertexBufferSize, (*QuadVertexStagingBuffer).Buffer);
        QuadVertexStagingBuffer->Capacity = s_Data.DefaultVertexBufferSize;
        s_Data.StagingStorage.StagingBuffers.push_back(std::move(QuadVertexStagingBuffer));
        ++s_Data.StagingStorage.CurrentStagingBufferIndex;
    }
    BufferUtils::CopyDataToBuffer(s_Data.StagingStorage.StagingBuffers[s_Data.StagingStorage.CurrentStagingBufferIndex]->Buffer, DataSize,
                                  s_Data.QuadVertexBufferBase);
    s_Data.StagingStorage.StagingBuffers[s_Data.StagingStorage.CurrentStagingBufferIndex]->Size += DataSize;

    s_Data.QuadVertexBuffers[s_Data.CurrentQuadVertexBufferIndex]->SetStagedData(
        (*s_Data.StagingStorage.StagingBuffers[s_Data.StagingStorage.CurrentStagingBufferIndex]).Buffer, DataSize);

    std::vector<VkDescriptorImageInfo> ImageInfos;
    for (uint32_t i = 0; i < VulkanRenderer2DStorage::MaxTextureSlots; ++i)
    {
        VkDescriptorImageInfo DescriptorImageInfo = {};
        DescriptorImageInfo.imageLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        if (s_Data.TextureSlots[i] && i < s_Data.CurrentTextureSlotIndex)
        {
            const auto Texture  = s_Data.TextureSlots[i];
            DescriptorImageInfo = Texture->GetImageDescriptorInfo();
        }
        else
        {
            DescriptorImageInfo.imageView = s_Data.QuadWhiteTexture->GetImage()->GetView();
            DescriptorImageInfo.sampler   = s_Data.QuadWhiteTexture->GetImage()->GetSampler();
        }

        ImageInfos.push_back(DescriptorImageInfo);
    }
    const auto TextureWriteSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
                                                                s_Data.QuadDescriptorSets[s_Data.CurrentDescriptorSetIndex],
                                                                (uint32_t)ImageInfos.size(), ImageInfos.data());
    vkUpdateDescriptorSets(m_Context.GetDevice()->GetLogicalDevice(), 1, &TextureWriteSet, 0, nullptr);

    const auto& Swapchain = m_Context.GetSwapchain();
    auto& CommandBuffer   = m_Context.GetGraphicsCommandPool()->GetCommandBuffer(Swapchain->GetCurrentFrameIndex());

    CommandBuffer.BindPipeline(s_Data.QuadPipeline);

    VkDeviceSize Offset = 0;
    CommandBuffer.BindVertexBuffers(0, 1, &s_Data.QuadVertexBuffers[s_Data.CurrentQuadVertexBufferIndex]->Get(), &Offset);
    CommandBuffer.BindIndexBuffer(s_Data.QuadIndexBuffer->Get(), 0, VK_INDEX_TYPE_UINT32);

    CommandBuffer.BindDescriptorSets(s_Data.QuadPipeline->GetLayout(), 0, 1, &s_Data.QuadDescriptorSets[s_Data.CurrentDescriptorSetIndex]);
    CommandBuffer.BindPushConstants(s_Data.QuadPipeline->GetLayout(), s_Data.QuadPipeline->GetPushConstantsShaderStageFlags(), 0,
                                    s_Data.QuadPipeline->GetPushConstantsSize(), &s_Data.PushConstants);

    CommandBuffer.DrawIndexed(s_Data.QuadIndexCount);

    ++Renderer::GetStats().DrawCalls;
    ++s_Data.CurrentDescriptorSetIndex;
    ++s_Data.CurrentQuadVertexBufferIndex;
}

void VulkanRenderer2D::Destroy()
{
    m_Context.GetDevice()->WaitDeviceOnFinish();

    vkDestroyDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), s_Data.QuadDescriptorSetLayout, nullptr);

    delete[] s_Data.QuadVertexBufferBase;
    s_Data.QuadWhiteTexture->Destroy();
    s_Data.QuadPipeline->Destroy();
    s_Data.VertexShader->DestroyModule();
    s_Data.FragmentShader->DestroyModule();

    s_Data.QuadIndexBuffer->Destroy();

    for (auto& QuadVertexBuffer : s_Data.QuadVertexBuffers)
        QuadVertexBuffer->Destroy();

    for (auto& QuadVertexStagingBuffer : s_Data.StagingStorage.StagingBuffers)
        BufferUtils::DestroyBuffer((*QuadVertexStagingBuffer).Buffer);
}

}  // namespace Gauntlet