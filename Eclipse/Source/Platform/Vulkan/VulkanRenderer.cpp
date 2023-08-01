#include "EclipsePCH.h"
#include "VulkanRenderer.h"

#include "Eclipse/Renderer/Camera/PerspectiveCamera.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanPipeline.h"
#include "VulkanDescriptors.h"

#include "VulkanUtility.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanTextureCube.h"
#include "VulkanBuffer.h"

#include "Eclipse/Renderer/Skybox.h"
#include "Eclipse/Renderer/Mesh.h"

namespace Eclipse
{
// TODO: Align
struct VulkanRendererStorage
{
    // Mesh
    Ref<VulkanPipeline> MeshPipeline;
    Ref<VulkanShader> MeshVertexShader;
    Ref<VulkanShader> MeshFragmentShader;
    Ref<VulkanTexture2D> MeshWhiteTexture;

    BufferLayout MeshVertexBufferLayout;
    Ref<VulkanDescriptorAllocator> MeshDescriptorAllocator;

    VkDescriptorSetLayout MeshDescriptorSetLayout;
    std::vector<VkDescriptorSet> MeshDescriptorSets;
    uint32_t CurrentDescriptorSetIndex = 0;

    glm::mat4 CameraProjectionMatrix = glm::mat4(1.0f);

    // Skybox
    Ref<Skybox> DefaultSkybox;
    Ref<VulkanPipeline> SkyboxPipeline;
    Ref<VulkanShader> SkyboxVertexShader;
    Ref<VulkanShader> SkyboxFragmentShader;

    BufferLayout SkyboxVertexBufferLayout;
    VkDescriptorSetLayout SkyboxDescriptorSetLayout;
    VkDescriptorSet SkyboxDescriptorSet;
    MeshPushConstants SkyboxPushConstants;
};

static VulkanRendererStorage s_Data;

VulkanRenderer::VulkanRenderer() : m_Context((VulkanContext&)VulkanContext::Get())
{
    Create();
}

void VulkanRenderer::Create()
{
    s_Data.MeshVertexBufferLayout = {
        {EShaderDataType::Vec3, "InPosition"},  //
        {EShaderDataType::Vec3, "InNormal"},    //
        {EShaderDataType::Vec4, "InColor"},     //
        {EShaderDataType::Vec2, "InTexCoord"},  //
        {EShaderDataType::Vec3, "Tangent"},     //
        {EShaderDataType::Vec3, "Bitangent"}    //
    };

    s_Data.MeshVertexShader.reset(new VulkanShader(std::string(ASSETS_PATH) + "Shaders/Mesh.vert.spv"));
    s_Data.MeshFragmentShader.reset(new VulkanShader(std::string(ASSETS_PATH) + "Shaders/Mesh.frag.spv"));

    uint32_t WhiteTexutreData = 0xffffffff;
    s_Data.MeshWhiteTexture.reset(new VulkanTexture2D(&WhiteTexutreData, sizeof(WhiteTexutreData), 1, 1));

    // Setting up things all the things that related to uniforms in shader
    s_Data.MeshDescriptorAllocator.reset(new VulkanDescriptorAllocator(m_Context.GetDevice()));

    const auto DiffuseTextureBinding =
        Utility::GetDescriptorSetLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    const auto NormalTextureBinding =
        Utility::GetDescriptorSetLayoutBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

    VkDescriptorSetLayoutBinding Bindings[] = {DiffuseTextureBinding, NormalTextureBinding};
    auto MeshDescriptorSetLayoutCreateInfo  = Utility::GetDescriptorSetLayoutCreateInfo(2, Bindings, 0);
    VK_CHECK(vkCreateDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), &MeshDescriptorSetLayoutCreateInfo, nullptr,
                                         &s_Data.MeshDescriptorSetLayout),
             "Failed to create descriptor set layout!");

    // Base mesh pipeline creation
    PipelineSpecification PipelineSpec = {};
    PipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
    PipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
    PipelineSpec.PrimitiveTopology     = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PipelineSpec.CullMode              = ECullMode::CULL_MODE_NONE;
    PipelineSpec.bDepthTest            = VK_TRUE;
    PipelineSpec.bDepthWrite           = VK_TRUE;
    PipelineSpec.DepthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;

    PipelineSpec.RenderPass   = m_Context.GetMainRenderPass();
    PipelineSpec.ShaderStages = {
        {s_Data.MeshVertexShader, EShaderStage::SHADER_STAGE_VERTEX},     //
        {s_Data.MeshFragmentShader, EShaderStage::SHADER_STAGE_FRAGMENT}  //
    };

    VkPushConstantRange PushConstants = {};
    PushConstants.offset              = 0;
    PushConstants.size                = sizeof(MeshPushConstants);
    PushConstants.stageFlags          = VK_SHADER_STAGE_VERTEX_BIT;
    PipelineSpec.PushConstantRanges   = {PushConstants};

    PipelineSpec.ShaderBindingDescriptions = {Utility::GetShaderBindingDescription(0, s_Data.MeshVertexBufferLayout.GetStride())};

    std::vector<VkVertexInputAttributeDescription> ShaderAttributeDescriptions;
    for (uint32_t i = 0; i < s_Data.MeshVertexBufferLayout.GetElements().size(); ++i)
    {
        const auto& CurrentBufferElement = s_Data.MeshVertexBufferLayout.GetElements()[i];
        ShaderAttributeDescriptions.emplace_back(Utility::GetShaderAttributeDescription(
            0, Utility::EclipseShaderDataTypeToVulkan(CurrentBufferElement.Type), i, CurrentBufferElement.Offset));
    }
    PipelineSpec.ShaderAttributeDescriptions = ShaderAttributeDescriptions;

    PipelineSpec.DescriptorSetLayouts = {s_Data.MeshDescriptorSetLayout};
    s_Data.MeshPipeline.reset(new VulkanPipeline(PipelineSpec));

    SetupSkybox();
}

void VulkanRenderer::BeginSceneImpl(const PerspectiveCamera& InCamera)
{
    s_Data.CameraProjectionMatrix = InCamera.GetViewProjectionMatrix();

    s_Data.SkyboxPushConstants.RenderMatrix =
        InCamera.GetProjectionMatrix() *
        glm::mat4(glm::mat3(InCamera.GetViewMatrix()));  // Removing 4th column of view martix which contains translation
}

void VulkanRenderer::BeginImpl()
{
    s_Data.MeshDescriptorSets.clear();
    s_Data.CurrentDescriptorSetIndex = 0;

    s_Data.MeshDescriptorAllocator->ResetPools();
}

void VulkanRenderer::SubmitMeshImpl(const Ref<Mesh>& InMesh, const glm::mat4& InTransformMatrix)
{
    MeshPushConstants PushConstants = {};
    PushConstants.RenderMatrix      = s_Data.CameraProjectionMatrix * InTransformMatrix;

    const auto& Swapchain = m_Context.GetSwapchain();
    auto& CommandBuffer   = m_Context.GetGraphicsCommandPool()->GetCommandBuffer(Swapchain->GetCurrentFrameIndex());
    CommandBuffer.BeginDebugLabel("3D", glm::vec4(0.5f, 0.0f, 0.0f, 1.0f));

    CommandBuffer.BindPipeline(s_Data.MeshPipeline);

    CommandBuffer.BindPushConstants(s_Data.MeshPipeline->GetLayout(), s_Data.MeshPipeline->GetPushConstantsShaderStageFlags(), 0,
                                    s_Data.MeshPipeline->GetPushConstantsSize(), &PushConstants);

    for (uint32_t i = 0; i < InMesh->GetVertexBuffers().size(); ++i)
    {
        if (s_Data.CurrentDescriptorSetIndex >= s_Data.MeshDescriptorSets.size())
        {
            VkDescriptorSet MeshSamplerDescriptorSet = VK_NULL_HANDLE;
            ELS_ASSERT(s_Data.MeshDescriptorAllocator->Allocate(&MeshSamplerDescriptorSet, s_Data.MeshDescriptorSetLayout),
                       "Failed to allocate descriptor sets!");
            s_Data.MeshDescriptorSets.push_back(MeshSamplerDescriptorSet);
        }

        std::vector<VkWriteDescriptorSet> Writes;
        if (InMesh->HasDiffuseTexture(i))
        {

            auto DiffuseImageInfo = std::static_pointer_cast<VulkanTexture2D>(InMesh->GetDiffuseTexture(i))->GetImageDescriptorInfo();
            const auto DiffuseTextureWriteSet =
                Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
                                               s_Data.MeshDescriptorSets[s_Data.CurrentDescriptorSetIndex], 1, &DiffuseImageInfo);

            Writes.push_back(DiffuseTextureWriteSet);
        }
        else
        {
            auto WhiteImageInfo = s_Data.MeshWhiteTexture->GetImageDescriptorInfo();
            const auto WhiteTextureWriteSet =
                Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
                                               s_Data.MeshDescriptorSets[s_Data.CurrentDescriptorSetIndex], 1, &WhiteImageInfo);

            Writes.push_back(WhiteTextureWriteSet);
        }

        const bool bHasNormalMaps = InMesh->HasNormalMapTexture(i);
        if (bHasNormalMaps)
        {
            auto NormalMapImageInfo = std::static_pointer_cast<VulkanTexture2D>(InMesh->GetNormalMapTexture(i))->GetImageDescriptorInfo();
            const auto NormalTextureWriteSet =
                Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                                               s_Data.MeshDescriptorSets[s_Data.CurrentDescriptorSetIndex], 1, &NormalMapImageInfo);

            Writes.push_back(NormalTextureWriteSet);
        }

        vkUpdateDescriptorSets(m_Context.GetDevice()->GetLogicalDevice(), static_cast<uint32_t>(Writes.size()), Writes.data(), 0,
                               VK_NULL_HANDLE);

        CommandBuffer.BindDescriptorSets(s_Data.MeshPipeline->GetLayout(), 0, 1,
                                         &s_Data.MeshDescriptorSets[s_Data.CurrentDescriptorSetIndex]);

        ++s_Data.CurrentDescriptorSetIndex;

        VkDeviceSize Offset = 0;
        CommandBuffer.BindVertexBuffers(0, 1, &std::static_pointer_cast<VulkanVertexBuffer>(InMesh->GetVertexBuffers()[i])->Get(), &Offset);

        CommandBuffer.BindIndexBuffer(std::static_pointer_cast<VulkanIndexBuffer>(InMesh->GetIndexBuffers()[i])->Get(), 0,
                                      VK_INDEX_TYPE_UINT32);
        CommandBuffer.DrawIndexed(std::static_pointer_cast<VulkanIndexBuffer>(InMesh->GetIndexBuffers()[i])->GetCount());
    }
    CommandBuffer.EndDebugLabel();
}

void VulkanRenderer::SetupSkybox()
{
    const auto SkyboxPath                = std::string(ASSETS_PATH) + "Textures/Skybox/Yokohama3/";
    const std::vector<std::string> Faces = {
        SkyboxPath + "right.jpg",   //
        SkyboxPath + "left.jpg",    //
        SkyboxPath + "bottom.jpg",  // Swapped because I flip the viewport's Y axis
        SkyboxPath + "top.jpg",     // Swapped
        SkyboxPath + "front.jpg",   //
        SkyboxPath + "back.jpg"     //
    };
    s_Data.DefaultSkybox.reset(new Skybox(Faces));

    const auto CubeMapTextureBinding =
        Utility::GetDescriptorSetLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

    auto SkyboxDescriptorSetLayoutCreateInfo = Utility::GetDescriptorSetLayoutCreateInfo(1, &CubeMapTextureBinding, 0);
    VK_CHECK(vkCreateDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), &SkyboxDescriptorSetLayoutCreateInfo, nullptr,
                                         &s_Data.SkyboxDescriptorSetLayout),
             "Failed to create descriptor set layout!");

    s_Data.SkyboxVertexShader.reset(new VulkanShader(std::string(ASSETS_PATH) + "Shaders/Skybox.vert.spv"));
    s_Data.SkyboxFragmentShader.reset(new VulkanShader(std::string(ASSETS_PATH) + "Shaders/Skybox.frag.spv"));

    s_Data.SkyboxVertexBufferLayout = {
        {EShaderDataType::Vec3, "InPosition"}  //
    };

    // Base skybox pipeline creation
    PipelineSpecification PipelineSpec = {};
    PipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
    PipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
    PipelineSpec.PrimitiveTopology     = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PipelineSpec.CullMode              = ECullMode::CULL_MODE_BACK;
    PipelineSpec.bDepthTest            = VK_TRUE;
    PipelineSpec.bDepthWrite           = VK_TRUE;
    PipelineSpec.DepthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;

    PipelineSpec.RenderPass   = m_Context.GetMainRenderPass();
    PipelineSpec.ShaderStages = {
        {s_Data.SkyboxVertexShader, EShaderStage::SHADER_STAGE_VERTEX},     //
        {s_Data.SkyboxFragmentShader, EShaderStage::SHADER_STAGE_FRAGMENT}  //
    };

    VkPushConstantRange PushConstants = {};
    PushConstants.offset              = 0;
    PushConstants.size                = sizeof(MeshPushConstants);
    PushConstants.stageFlags          = VK_SHADER_STAGE_VERTEX_BIT;
    PipelineSpec.PushConstantRanges   = {PushConstants};

    PipelineSpec.ShaderBindingDescriptions = {Utility::GetShaderBindingDescription(0, s_Data.SkyboxVertexBufferLayout.GetStride())};

    std::vector<VkVertexInputAttributeDescription> ShaderAttributeDescriptions;
    for (uint32_t i = 0; i < s_Data.SkyboxVertexBufferLayout.GetElements().size(); ++i)
    {
        const auto& CurrentBufferElement = s_Data.SkyboxVertexBufferLayout.GetElements()[i];
        ShaderAttributeDescriptions.emplace_back(Utility::GetShaderAttributeDescription(
            0, Utility::EclipseShaderDataTypeToVulkan(CurrentBufferElement.Type), i, CurrentBufferElement.Offset));
    }
    PipelineSpec.ShaderAttributeDescriptions = ShaderAttributeDescriptions;

    PipelineSpec.DescriptorSetLayouts = {s_Data.SkyboxDescriptorSetLayout};
    s_Data.SkyboxPipeline.reset(new VulkanPipeline(PipelineSpec));
}

void VulkanRenderer::DrawSkybox()
{
    auto& CommandBuffer = m_Context.GetGraphicsCommandPool()->GetCommandBuffer(m_Context.GetSwapchain()->GetCurrentFrameIndex());
    CommandBuffer.BeginDebugLabel("Skybox", glm::vec4(0.0f, 0.0f, 0.25f, 1.0f));

    CommandBuffer.BindPipeline(s_Data.SkyboxPipeline);

    static constexpr uint32_t s_MaxSkyboxSize = 460.0f;
    s_Data.SkyboxPushConstants.RenderMatrix *= glm::scale(glm::mat4(1.0f), glm::vec3(s_MaxSkyboxSize));

    CommandBuffer.BindPushConstants(s_Data.SkyboxPipeline->GetLayout(), s_Data.SkyboxPipeline->GetPushConstantsShaderStageFlags(), 0,
                                    s_Data.SkyboxPipeline->GetPushConstantsSize(), &s_Data.SkyboxPushConstants);

    ELS_ASSERT(s_Data.MeshDescriptorAllocator->Allocate(&s_Data.SkyboxDescriptorSet, s_Data.SkyboxDescriptorSetLayout),
               "Failed to allocate descriptor sets!");

    const auto& CubeMesh = s_Data.DefaultSkybox->GetCubeMesh();
    std::vector<VkWriteDescriptorSet> Writes;
    if (auto& CubeMapTexture = std::static_pointer_cast<VulkanTextureCube>(s_Data.DefaultSkybox->GetCubeMapTexture()))
    {
        auto CubeMapTextureImageInfo      = CubeMapTexture->GetImage()->GetDescriptorInfo();
        const auto CubeMapTextureWriteSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
                                                                           s_Data.SkyboxDescriptorSet, 1, &CubeMapTextureImageInfo);

        Writes.push_back(CubeMapTextureWriteSet);
    }
    else
    {
        auto WhiteImageInfo = s_Data.MeshWhiteTexture->GetImageDescriptorInfo();
        const auto WhiteTextureWriteSet =
            Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, s_Data.SkyboxDescriptorSet, 1, &WhiteImageInfo);

        Writes.push_back(WhiteTextureWriteSet);
    }

    vkUpdateDescriptorSets(m_Context.GetDevice()->GetLogicalDevice(), static_cast<uint32_t>(Writes.size()), Writes.data(), 0,
                           VK_NULL_HANDLE);

    CommandBuffer.BindDescriptorSets(s_Data.SkyboxPipeline->GetLayout(), 0, 1, &s_Data.SkyboxDescriptorSet);

    const auto& VkVertexBuffer = std::static_pointer_cast<VulkanVertexBuffer>(CubeMesh->GetVertexBuffers()[0]);
    VkDeviceSize Offset        = 0;
    CommandBuffer.BindVertexBuffers(0, 1, &VkVertexBuffer->Get(), &Offset);

    const auto& VkIndexBuffer = std::static_pointer_cast<VulkanIndexBuffer>(CubeMesh->GetIndexBuffers()[0]);
    CommandBuffer.BindIndexBuffer(VkIndexBuffer->Get(), 0, VK_INDEX_TYPE_UINT32);
    CommandBuffer.DrawIndexed(VkIndexBuffer->GetCount());

    CommandBuffer.EndDebugLabel();
}

void VulkanRenderer::DestroySkybox()
{
    vkDestroyDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), s_Data.SkyboxDescriptorSetLayout, nullptr);

    s_Data.SkyboxVertexShader->DestroyModule();
    s_Data.SkyboxFragmentShader->DestroyModule();
    s_Data.SkyboxPipeline->Destroy();

    s_Data.DefaultSkybox->Destroy();
}

void VulkanRenderer::FlushImpl()
{
    DrawSkybox();
}

void VulkanRenderer::Destroy()
{
    m_Context.GetDevice()->WaitDeviceOnFinish();

    vkDestroyDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), s_Data.MeshDescriptorSetLayout, nullptr);
    s_Data.MeshDescriptorAllocator->Destroy();

    s_Data.MeshPipeline->Destroy();
    s_Data.MeshVertexShader->DestroyModule();
    s_Data.MeshFragmentShader->DestroyModule();
    s_Data.MeshWhiteTexture->Destroy();

    DestroySkybox();
}

}  // namespace Eclipse