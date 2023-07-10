#include "EclipsePCH.h"
#include "VulkanRenderer2D.h"

#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanShader.h"
#include "VulkanPipeline.h"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"
#include "VulkanRenderPass.h"
#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"

#define MAX_TEXTURES_PER_DRAW_CALL 32

namespace Eclipse
{

struct VulkanRenderer2DStorage
{
    const uint32_t MaxQuads = 10000;
    const uint32_t MaxVertices = MaxQuads * 4;
    const uint32_t MaxIndices = MaxQuads * 6;

    Ref<VulkanVertexBuffer> QuadVertexBuffer;
    BufferLayout VertexBufferLayout;

    Ref<VulkanIndexBuffer> QuadIndexBuffer;
    uint32_t QuadIndexCount = 0;

    Ref<VulkanPipeline> QuadPipeline;

    VkDescriptorPool QuadDescriptorPool;
    VkDescriptorSetLayout QuadDescriptorSetLayout;
    VkDescriptorSet QuadDescriptorSet;

    Ref<VulkanTexture2D> QuadWhiteTexture;  // Texture Slot 0 = white tex
    uint32_t CurrentTextureSlotIndex = 1;   // 0 slot already tied with white texture

    std::array<Ref<VulkanTexture2D>, MAX_TEXTURES_PER_DRAW_CALL> TextureSlots;

    Ref<VulkanShader> VertexShader;
    Ref<VulkanShader> FragmentShader;

    QuadVertex* QuadVertexBufferBase = nullptr;
    QuadVertex* QuadVertexBufferPtr = nullptr;

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

        BufferInfo VertexBufferInfo = {};
        VertexBufferInfo.Usage = EBufferUsageFlags::VERTEX_BUFFER;
        VertexBufferInfo.Layout = s_Data.VertexBufferLayout;
        VertexBufferInfo.Count = s_Data.MaxVertices;
        s_Data.QuadVertexBuffer.reset(new VulkanVertexBuffer(VertexBufferInfo));
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
        IndexBufferInfo.Usage = EBufferUsageFlags::INDEX_BUFFER;
        IndexBufferInfo.Count = s_Data.MaxIndices;
        IndexBufferInfo.Size = s_Data.MaxIndices * sizeof(uint32_t);
        IndexBufferInfo.Data = QuadIndices;
        s_Data.QuadIndexBuffer.reset(new VulkanIndexBuffer(IndexBufferInfo));

        delete[] QuadIndices;
    }

    // Default white texture
    uint32_t WhiteTextureData = 0xffffffff;
    s_Data.QuadWhiteTexture.reset(new VulkanTexture2D(&WhiteTextureData, sizeof(uint32_t), 1, 1));

    s_Data.TextureSlots[0] = s_Data.QuadWhiteTexture;

    // Setting up things all the things that related to uniforms in shader
    const auto TextureBinding = Utility::GetDescriptorSetLayoutBinding(
        0, MAX_TEXTURES_PER_DRAW_CALL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

    // Vulkan 1.2 Features for texture batching in my case
    VkDescriptorBindingFlags BindingFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT /*| VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT*/;

    VkDescriptorSetLayoutBindingFlagsCreateInfo BindingFlagsInfo = {};
    BindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    BindingFlagsInfo.bindingCount = 1;
    BindingFlagsInfo.pBindingFlags = &BindingFlags;

    VkDescriptorSetLayoutCreateInfo QuadDescriptorSetLayoutCreateInfo = {};
    QuadDescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    QuadDescriptorSetLayoutCreateInfo.bindingCount = 1;
    QuadDescriptorSetLayoutCreateInfo.pBindings = &TextureBinding;
    QuadDescriptorSetLayoutCreateInfo.pNext = &BindingFlagsInfo;
    // QuadDescriptorSetLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

    VK_CHECK(vkCreateDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), &QuadDescriptorSetLayoutCreateInfo, nullptr,
                                         &s_Data.QuadDescriptorSetLayout),
             "Failed ot create descriptor set layout!");

    VkDescriptorPoolSize PoolSizes[] = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, FRAMES_IN_FLIGHT}};

    VkDescriptorPoolCreateInfo QuadDescriptorPoolCreateInfo = {};
    QuadDescriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    QuadDescriptorPoolCreateInfo.poolSizeCount = 1;
    QuadDescriptorPoolCreateInfo.maxSets = 1;
    QuadDescriptorPoolCreateInfo.pPoolSizes = PoolSizes;
    // QuadDescriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

    VK_CHECK(vkCreateDescriptorPool(m_Context.GetDevice()->GetLogicalDevice(), &QuadDescriptorPoolCreateInfo, nullptr,
                                    &s_Data.QuadDescriptorPool),
             "Failed to create descriptor pool!");

    VkDescriptorSetAllocateInfo QuadDescriptorSetAllocateInfo = {};
    QuadDescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    QuadDescriptorSetAllocateInfo.descriptorSetCount = 1;
    QuadDescriptorSetAllocateInfo.pSetLayouts = &s_Data.QuadDescriptorSetLayout;
    QuadDescriptorSetAllocateInfo.descriptorPool = s_Data.QuadDescriptorPool;

    VK_CHECK(vkAllocateDescriptorSets(m_Context.GetDevice()->GetLogicalDevice(), &QuadDescriptorSetAllocateInfo, &s_Data.QuadDescriptorSet),
             "Failed to allocate descriptor set!");

    // Default 2D graphics pipeline creation
    {
        PipelineSpecification PipelineSpec = {};
        PipelineSpec.RenderPass = m_Context.GetGlobalRenderPass()->Get();

        const std::vector<VkVertexInputBindingDescription> ShaderBindingDescriptions = {
            Utility::GetShaderBindingDescription(0, s_Data.VertexBufferLayout.GetStride())};
        PipelineSpec.ShaderBindingDescriptions = ShaderBindingDescriptions;

        std::vector<VkVertexInputAttributeDescription> ShaderAttributeDescriptions(s_Data.VertexBufferLayout.GetElements().size());
        for (uint32_t i = 0; i < ShaderAttributeDescriptions.size(); ++i)
        {
            ShaderAttributeDescriptions[i] = Utility::GetShaderAttributeDescription(
                0, Utility::EclipseShaderDataTypeToVulkan(s_Data.VertexBufferLayout.GetElements()[i].Type), i,
                s_Data.VertexBufferLayout.GetElements()[i].Offset);
        }

        PipelineSpec.ShaderAttributeDescriptions = ShaderAttributeDescriptions;

        s_Data.VertexShader = Ref<VulkanShader>(new VulkanShader("Resources/Shaders/FlatColor.vert.spv"));
        s_Data.FragmentShader = Ref<VulkanShader>(new VulkanShader("Resources/Shaders/FlatColor.frag.spv"));

        std::vector<PipelineSpecification::ShaderStage> ShaderStages(2);
        ShaderStages[0].Stage = EShaderStage::SHADER_STAGE_VERTEX;
        ShaderStages[0].Shader = s_Data.VertexShader;

        ShaderStages[1].Stage = EShaderStage::SHADER_STAGE_FRAGMENT;
        ShaderStages[1].Shader = s_Data.FragmentShader;

        PipelineSpec.ShaderStages = ShaderStages;

        VkPushConstantRange PushConstants = {};
        PushConstants.offset = 0;
        PushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        PushConstants.size = sizeof(MeshPushConstants);
        PipelineSpec.PushConstantRanges = {PushConstants};

        PipelineSpec.DescriptorSetLayouts = {s_Data.QuadDescriptorSetLayout};

        s_Data.QuadPipeline.reset(new VulkanPipeline(PipelineSpec));
    }

    m_Context.AddResizeCallback(
        [this]
        {
            s_Data.QuadPipeline->Destroy();
            s_Data.QuadPipeline->SetRenderPass(m_Context.GetGlobalRenderPass()->Get());
            s_Data.QuadPipeline->Create();
        });
}

void VulkanRenderer2D::BeginImpl()
{
    s_Data.CurrentTextureSlotIndex = 1;
    s_Data.QuadIndexCount = 0;
    s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

    // m_Context.GetDevice()->WaitDeviceOnFinish();
    // VK_CHECK(vkResetDescriptorPool(m_Context.GetDevice()->GetLogicalDevice(), s_Data.QuadDescriptorPool, 0),
    //          "Failed to reset descriptor pool!");
}

void VulkanRenderer2D::SetClearColorImpl(const glm::vec4& InColor)
{
    m_Context.SetClearColor(InColor);
}

void VulkanRenderer2D::BeginSceneImpl(const OrthographicCamera& InCamera)
{
    s_Data.CameraProjectionMatrix = InCamera.GetProjectionMatrix();
}

void VulkanRenderer2D::DrawQuadImpl(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec4& InColor)
{
    DrawRotatedQuadImpl(InPosition, InSize, glm::vec3(0.0f), InColor);
}

void VulkanRenderer2D::DrawRotatedQuadImpl(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec3& InRotation,
                                           const glm::vec4& InColor)
{
    const float TextureId = 0.0f;  // White by default

    s_Data.QuadVertexBufferPtr->Position = InPosition;
    s_Data.QuadVertexBufferPtr->Color = InColor;
    s_Data.QuadVertexBufferPtr->TexCoord = {0.0f, 0.0f};
    s_Data.QuadVertexBufferPtr->TextureId = TextureId;
    ++s_Data.QuadVertexBufferPtr;

    s_Data.QuadVertexBufferPtr->Position = {InPosition.x + InSize.x, InPosition.y, InPosition.z};
    s_Data.QuadVertexBufferPtr->Color = InColor;
    s_Data.QuadVertexBufferPtr->TexCoord = {1.0f, 0.0f};
    s_Data.QuadVertexBufferPtr->TextureId = TextureId;
    ++s_Data.QuadVertexBufferPtr;

    s_Data.QuadVertexBufferPtr->Position = {InPosition.x + InSize.x, InPosition.y + InSize.y, InPosition.z};
    s_Data.QuadVertexBufferPtr->Color = InColor;
    s_Data.QuadVertexBufferPtr->TexCoord = {1.0f, 1.0f};
    s_Data.QuadVertexBufferPtr->TextureId = TextureId;
    ++s_Data.QuadVertexBufferPtr;

    s_Data.QuadVertexBufferPtr->Position = {InPosition.x, InPosition.y + InSize.y, InPosition.z};
    s_Data.QuadVertexBufferPtr->Color = InColor;
    s_Data.QuadVertexBufferPtr->TexCoord = {0.0f, 1.0f};
    s_Data.QuadVertexBufferPtr->TextureId = TextureId;
    ++s_Data.QuadVertexBufferPtr;

    s_Data.QuadIndexCount += 6;

    s_Data.PushConstants.RenderMatrix = s_Data.CameraProjectionMatrix;
    s_Data.PushConstants.Color = InColor;
}

void VulkanRenderer2D::DrawTexturedQuadImpl(const glm::vec3& InPosition, const glm::vec2& InSize, const Ref<Texture2D>& InTexture,
                                            const glm::vec4& InBlendColor)
{
    float TextureIndex = 0.0f;
    for (uint32_t i = 0; i < s_Data.CurrentTextureSlotIndex; ++i)
    {
        if (s_Data.TextureSlots[i] == InTexture)
        {
            TextureIndex = (float)i;
            break;
        }
    }

    if (TextureIndex == 0.0f)
    {
        const auto VulkanTexture = std::static_pointer_cast<VulkanTexture2D>(InTexture);
        s_Data.TextureSlots[s_Data.CurrentTextureSlotIndex] = VulkanTexture;
        TextureIndex = s_Data.CurrentTextureSlotIndex;
        ++s_Data.CurrentTextureSlotIndex;
    }

    s_Data.QuadVertexBufferPtr->Position = InPosition;
    s_Data.QuadVertexBufferPtr->Color = InBlendColor;
    s_Data.QuadVertexBufferPtr->TexCoord = {0.0f, 0.0f};
    s_Data.QuadVertexBufferPtr->TextureId = TextureIndex;
    ++s_Data.QuadVertexBufferPtr;

    s_Data.QuadVertexBufferPtr->Position = {InPosition.x + InSize.x, InPosition.y, InPosition.z};
    s_Data.QuadVertexBufferPtr->Color = InBlendColor;
    s_Data.QuadVertexBufferPtr->TexCoord = {1.0f, 0.0f};
    s_Data.QuadVertexBufferPtr->TextureId = TextureIndex;
    ++s_Data.QuadVertexBufferPtr;

    s_Data.QuadVertexBufferPtr->Position = {InPosition.x + InSize.x, InPosition.y + InSize.y, InPosition.z};
    s_Data.QuadVertexBufferPtr->Color = InBlendColor;
    s_Data.QuadVertexBufferPtr->TexCoord = {1.0f, 1.0f};
    s_Data.QuadVertexBufferPtr->TextureId = TextureIndex;
    ++s_Data.QuadVertexBufferPtr;

    s_Data.QuadVertexBufferPtr->Position = {InPosition.x, InPosition.y + InSize.y, InPosition.z};
    s_Data.QuadVertexBufferPtr->Color = InBlendColor;
    s_Data.QuadVertexBufferPtr->TexCoord = {0.0f, 1.0f};
    s_Data.QuadVertexBufferPtr->TextureId = TextureIndex;
    ++s_Data.QuadVertexBufferPtr;

    s_Data.QuadIndexCount += 6;

    s_Data.PushConstants.RenderMatrix = s_Data.CameraProjectionMatrix;
    s_Data.PushConstants.Color = InBlendColor;
}

void VulkanRenderer2D::FlushImpl()
{
    if (s_Data.QuadIndexCount <= 0) return;

    /*VkDescriptorSetAllocateInfo QuadDescriptorSetAllocateInfo = {};
    QuadDescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    QuadDescriptorSetAllocateInfo.descriptorSetCount = 1;
    QuadDescriptorSetAllocateInfo.pSetLayouts = &s_Data.QuadDescriptorSetLayout;
    QuadDescriptorSetAllocateInfo.descriptorPool = s_Data.QuadDescriptorPool;

    VK_CHECK(vkAllocateDescriptorSets(m_Context.GetDevice()->GetLogicalDevice(), &QuadDescriptorSetAllocateInfo, &s_Data.QuadDescriptorSet),
             "Failed to allocate descriptor set!");*/

    std::vector<VkDescriptorImageInfo> ImageInfos;
    for (uint32_t i = 0; i < MAX_TEXTURES_PER_DRAW_CALL; ++i)
    {
        VkDescriptorImageInfo DescriptorImageInfo = {};
        DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        if (s_Data.TextureSlots[i] && i < s_Data.CurrentTextureSlotIndex)
        {
            const auto Texture = s_Data.TextureSlots[i];
            DescriptorImageInfo.imageView = Texture->GetImage()->GetView();
            DescriptorImageInfo.sampler = Texture->GetSampler();
        }
        else
        {
            DescriptorImageInfo.imageView = s_Data.QuadWhiteTexture->GetImage()->GetView();
            DescriptorImageInfo.sampler = s_Data.QuadWhiteTexture->GetSampler();
        }

        ImageInfos.push_back(DescriptorImageInfo);
    }
    const auto TextureWriteSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, s_Data.QuadDescriptorSet,
                                                                ImageInfos.size(), ImageInfos.data());
    vkUpdateDescriptorSets(m_Context.GetDevice()->GetLogicalDevice(), 1, &TextureWriteSet, 0, nullptr);

    // Cherno's version
    /* std::vector<VkDescriptorImageInfo> TextureInfos;
     for (uint32_t i = 0; i < MAX_TEXTURES_PER_DRAW_CALL; ++i)
     {
         VkDescriptorImageInfo DescriptorImageInfo = {};
         DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

         if (s_Data.TextureSlots[i] && i < s_Data.CurrentTextureSlotIndex)
         {
             const auto Texture = s_Data.TextureSlots[i];
             DescriptorImageInfo.imageView = Texture->GetImage()->GetView();
             DescriptorImageInfo.sampler = Texture->GetSampler();
         }
         else
         {
             DescriptorImageInfo.imageView = s_Data.QuadWhiteTexture->GetImage()->GetView();
             DescriptorImageInfo.sampler = s_Data.QuadWhiteTexture->GetSampler();
         }

         TextureInfos.emplace_back(DescriptorImageInfo);
     }

     const auto TextureSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, s_Data.QuadDescriptorSet,
                                                            s_Data.CurrentTextureSlotIndex, TextureInfos.data());

     vkUpdateDescriptorSets(m_Context.GetDevice()->GetLogicalDevice(), 1, &TextureSet, 0, nullptr);*/

    /*   for (uint32_t i = 0; i < MAX_TEXTURES_PER_DRAW_CALL; ++i)
       {
           VkDescriptorImageInfo DescriptorImageInfo = {};
           DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
           if (s_Data.TextureSlots[i] && i < s_Data.CurrentTextureSlotIndex)
           {
               const auto Texture = s_Data.TextureSlots[i];
               DescriptorImageInfo.imageView = Texture->GetImage()->GetView();
               DescriptorImageInfo.sampler = Texture->GetSampler();
           }
           else
           {
               DescriptorImageInfo.imageView = s_Data.QuadWhiteTexture->GetImage()->GetView();
               DescriptorImageInfo.sampler = s_Data.QuadWhiteTexture->GetSampler();
           }
           const auto TextureWriteSet =
               Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, s_Data.QuadDescriptorSet, 1,
       &DescriptorImageInfo); vkUpdateDescriptorSets(m_Context.GetDevice()->GetLogicalDevice(), 1, &TextureWriteSet, 0, VK_NULL_HANDLE);
       }*/

    const uint32_t DataSize = (uint8_t*)s_Data.QuadVertexBufferPtr - (uint8_t*)s_Data.QuadVertexBufferBase;
    s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, DataSize);

    const auto& Swapchain = m_Context.GetSwapchain();
    auto& CommandBuffer = m_Context.GetGraphicsCommandPool()->GetCommandBuffer(Swapchain->GetCurrentFrameIndex());

    CommandBuffer.BindPipeline(s_Data.QuadPipeline->Get());

    VkDeviceSize Offset = 0;
    CommandBuffer.BindVertexBuffers(0, 1, &s_Data.QuadVertexBuffer->Get(), &Offset);
    CommandBuffer.BindIndexBuffer(s_Data.QuadIndexBuffer->Get(), 0, VK_INDEX_TYPE_UINT32);

    CommandBuffer.BindDescriptorSets(s_Data.QuadPipeline->GetLayout(), 0, 1, &s_Data.QuadDescriptorSet);
    CommandBuffer.BindPushConstants(s_Data.QuadPipeline->GetLayout(), s_Data.QuadPipeline->GetPushConstantsShaderStageFlags(), 0,
                                    s_Data.QuadPipeline->GetPushConstantsSize(), &s_Data.PushConstants);

    CommandBuffer.DrawIndexed(s_Data.QuadIndexCount);
}

void VulkanRenderer2D::Destroy()
{
    m_Context.GetDevice()->WaitDeviceOnFinish();

    delete[] s_Data.QuadVertexBufferBase;

    s_Data.QuadWhiteTexture->Destroy();

    vkDestroyDescriptorPool(m_Context.GetDevice()->GetLogicalDevice(), s_Data.QuadDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), s_Data.QuadDescriptorSetLayout, nullptr);

    s_Data.VertexShader->DestroyModule();
    s_Data.FragmentShader->DestroyModule();

    s_Data.QuadVertexBuffer->Destroy();
    s_Data.QuadIndexBuffer->Destroy();

    s_Data.QuadPipeline->Destroy();
}

}  // namespace Eclipse