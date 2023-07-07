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
#include "VulkanDescriptors.h"

namespace Eclipse
{
struct VulkanRenderer2DStorage
{
    // Per draw call limits
    const uint32_t MaxQuads = 10000;
    const uint32_t MaxVertices = 10000 * 4;
    const uint32_t MaxIndices = 10000 * 6;

    Ref<VulkanVertexBuffer> QuadVertexBuffer;
    BufferLayout VertexBufferLayout;

    Ref<VulkanIndexBuffer> QuadIndexBuffer;
    uint32_t QuadIndexCount = 0;

    Ref<VulkanPipeline> QuadPipeline;
    VkDescriptorPool QuadDescriptorPool;

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
        s_Data.VertexBufferLayout = {{EShaderDataType::Vec3, "InPostion"},    //
                                     {EShaderDataType::Vec3, "InNormal"},     //
                                     {EShaderDataType::Vec4, "InColor"},      //
                                     {EShaderDataType::Vec2, "InTexCoord"}};  //

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

    s_Data.QuadPipeline.reset(new VulkanPipeline(PipelineSpec));
}

void VulkanRenderer2D::BeginImpl()
{
    s_Data.QuadIndexCount = 0;
    s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;
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
    s_Data.QuadVertexBufferPtr->Position = InPosition;
    s_Data.QuadVertexBufferPtr->Color = InColor;
    s_Data.QuadVertexBufferPtr->TexCoord = {0.0f, 0.0f};
    ++s_Data.QuadVertexBufferPtr;

    s_Data.QuadVertexBufferPtr->Position = {InPosition.x + InSize.x, InPosition.y, InPosition.z};
    s_Data.QuadVertexBufferPtr->Color = InColor;
    s_Data.QuadVertexBufferPtr->TexCoord = {1.0f, 0.0f};
    ++s_Data.QuadVertexBufferPtr;

    s_Data.QuadVertexBufferPtr->Position = {InPosition.x + InSize.x, InPosition.y + InSize.y, InPosition.z};
    s_Data.QuadVertexBufferPtr->Color = InColor;
    s_Data.QuadVertexBufferPtr->TexCoord = {1.0f, 1.0f};
    ++s_Data.QuadVertexBufferPtr;

    s_Data.QuadVertexBufferPtr->Position = {InPosition.x, InPosition.y + InSize.y, InPosition.z};
    s_Data.QuadVertexBufferPtr->Color = InColor;
    s_Data.QuadVertexBufferPtr->TexCoord = {0.0f, 1.0f};
    ++s_Data.QuadVertexBufferPtr;

    s_Data.QuadIndexCount += 6;

    s_Data.PushConstants.RenderMatrix = s_Data.CameraProjectionMatrix;
    s_Data.PushConstants.Color = InColor;
}

void VulkanRenderer2D::FlushImpl()
{
    const uint32_t DataSize = (uint8_t*)s_Data.QuadVertexBufferPtr - (uint8_t*)s_Data.QuadVertexBufferBase;
    if (DataSize == 0) return;

    s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, DataSize);

    const auto& Swapchain = m_Context.GetSwapchain();
    auto& CommandBuffer = m_Context.GetGraphicsCommandPool()->GetCommandBuffer(Swapchain->GetCurrentFrameIndex());

    CommandBuffer.BindPipeline(s_Data.QuadPipeline->Get());

    VkDeviceSize Offset = 0;
    CommandBuffer.BindVertexBuffers(0, 1, &s_Data.QuadVertexBuffer->Get(), &Offset);
    CommandBuffer.BindIndexBuffer(s_Data.QuadIndexBuffer->Get(), 0, VK_INDEX_TYPE_UINT32);

    CommandBuffer.BindPushConstants(s_Data.QuadPipeline->GetLayout(), s_Data.QuadPipeline->GetPushConstantsShaderStageFlags(), 0,
                                    s_Data.QuadPipeline->GetPushConstantsSize(), &s_Data.PushConstants);

    CommandBuffer.DrawIndexed(s_Data.QuadIndexCount);
}

void VulkanRenderer2D::Destroy()
{
    const auto& Context = (VulkanContext&)VulkanContext::Get();
    Context.GetDevice()->WaitDeviceOnFinish();

    delete[] s_Data.QuadVertexBufferBase;

    s_Data.FragmentShader->DestroyModule();
    s_Data.VertexShader->DestroyModule();

    s_Data.QuadVertexBuffer->Destroy();

    s_Data.QuadIndexBuffer->Destroy();

    s_Data.QuadPipeline->Destroy();
}

}  // namespace Eclipse