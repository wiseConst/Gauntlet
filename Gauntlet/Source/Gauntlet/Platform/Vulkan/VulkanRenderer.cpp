#include "GauntletPCH.h"
#include "VulkanRenderer.h"

#include "Gauntlet/Renderer/Camera/PerspectiveCamera.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanPipeline.h"
#include "VulkanDescriptors.h"
#include "VulkanFramebuffer.h"

#include "VulkanUtility.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanTextureCube.h"
#include "VulkanBuffer.h"

#include "Gauntlet/Renderer/Skybox.h"
#include "Gauntlet/Renderer/Mesh.h"

namespace Gauntlet
{

struct VulkanRendererStorage
{
    // Framebuffers && RenderPasses
    Ref<VulkanFramebuffer> PostProcessFramebuffer = nullptr;

    // Mesh
    Ref<VulkanPipeline> MeshPipeline      = nullptr;
    Ref<VulkanShader> MeshVertexShader    = nullptr;
    Ref<VulkanShader> MeshFragmentShader  = nullptr;
    Ref<VulkanTexture2D> MeshWhiteTexture = nullptr;

    BufferLayout MeshVertexBufferLayout;

    VkDescriptorSetLayout MeshDescriptorSetLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> MeshDescriptorSets;
    uint32_t CurrentDescriptorSetIndex = 0;

    // Camera UBO
    std::vector<AllocatedBuffer> UniformCameraDataBuffers;
    std::vector<void*> MappedUniformCameraDataBuffers;
    CameraDataBuffer MeshCameraDataBuffer;

    // Skybox
    Ref<Skybox> DefaultSkybox              = nullptr;
    Ref<VulkanPipeline> SkyboxPipeline     = nullptr;
    Ref<VulkanShader> SkyboxVertexShader   = nullptr;
    Ref<VulkanShader> SkyboxFragmentShader = nullptr;

    BufferLayout SkyboxVertexBufferLayout;
    VkDescriptorSetLayout SkyboxDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet SkyboxDescriptorSet             = VK_NULL_HANDLE;
    MeshPushConstants SkyboxPushConstants;

    // UI
    VkDescriptorSetLayout ImageDescriptorSetLayout = VK_NULL_HANDLE;

    // Misc
    VulkanCommandBuffer* CurrentCommandBuffer = nullptr;
    Ref<VulkanPipeline> MeshWireframePipeline = nullptr;
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
        {EShaderDataType::Vec3, "InTangent"},   //
        {EShaderDataType::Vec3, "InBitangent"}  //
    };

    s_Data.MeshVertexShader.reset(new VulkanShader(std::string(ASSETS_PATH) + "Shaders/Mesh.vert.spv"));
    s_Data.MeshFragmentShader.reset(new VulkanShader(std::string(ASSETS_PATH) + "Shaders/Mesh.frag.spv"));

    const auto TextureDescriptorSetLayoutBinding =
        Utility::GetDescriptorSetLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutCreateInfo ImageDescriptorSetLayoutCreateInfo =
        Utility::GetDescriptorSetLayoutCreateInfo(1, &TextureDescriptorSetLayoutBinding);
    VK_CHECK(vkCreateDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), &ImageDescriptorSetLayoutCreateInfo, nullptr,
                                         &s_Data.ImageDescriptorSetLayout),
             "Failed to create texture(UI) descriptor set layout!");

    uint32_t WhiteTexutreData = 0xffffffff;
    s_Data.MeshWhiteTexture.reset(new VulkanTexture2D(&WhiteTexutreData, sizeof(WhiteTexutreData), 1, 1));

    const auto DiffuseTextureBinding =
        Utility::GetDescriptorSetLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    const auto NormalTextureBinding =
        Utility::GetDescriptorSetLayoutBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    const auto EmissiveTextureBinding =
        Utility::GetDescriptorSetLayoutBinding(2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    const auto EnvironmentMapBinding =
        Utility::GetDescriptorSetLayoutBinding(3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    const auto CameraDataBufferBinding =
        Utility::GetDescriptorSetLayoutBinding(4, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);

    VkDescriptorSetLayoutBinding Bindings[] = {DiffuseTextureBinding, NormalTextureBinding, EmissiveTextureBinding, EnvironmentMapBinding,
                                               CameraDataBufferBinding};
    auto MeshDescriptorSetLayoutCreateInfo  = Utility::GetDescriptorSetLayoutCreateInfo(5, Bindings);
    VK_CHECK(vkCreateDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), &MeshDescriptorSetLayoutCreateInfo, nullptr,
                                         &s_Data.MeshDescriptorSetLayout),
             "Failed to create mesh descriptor set layout!");

    FramebufferSpecification PostProcessFramebufferSpec = {};
    PostProcessFramebufferSpec.ClearColor               = glm::vec4(glm::vec3(0.1f), 1.0f);
    PostProcessFramebufferSpec.ColorLoadOp              = ELoadOp::CLEAR;  // Should it be LOAD?
    PostProcessFramebufferSpec.ColorStoreOp             = EStoreOp::STORE;
    PostProcessFramebufferSpec.DepthLoadOp              = ELoadOp::CLEAR;

    s_Data.PostProcessFramebuffer.reset(new VulkanFramebuffer(PostProcessFramebufferSpec));

    // Base mesh pipeline creation
    PipelineSpecification PipelineSpec = {};
    PipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
    PipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
    PipelineSpec.PrimitiveTopology     = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PipelineSpec.CullMode              = ECullMode::CULL_MODE_NONE;  // BACK?
    PipelineSpec.bDepthTest            = VK_TRUE;
    PipelineSpec.bDepthWrite           = VK_TRUE;
    PipelineSpec.DepthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;

    PipelineSpec.TargetFramebuffer = s_Data.PostProcessFramebuffer;
    PipelineSpec.ShaderStages      = {
        {s_Data.MeshVertexShader, EShaderStage::SHADER_STAGE_VERTEX},     //
        {s_Data.MeshFragmentShader, EShaderStage::SHADER_STAGE_FRAGMENT}  //
    };

    PipelineSpec.PushConstantRanges        = {Utility::GetPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(MeshPushConstants))};
    PipelineSpec.ShaderBindingDescriptions = {Utility::GetShaderBindingDescription(0, s_Data.MeshVertexBufferLayout.GetStride())};

    std::vector<VkVertexInputAttributeDescription> ShaderAttributeDescriptions;
    for (uint32_t i = 0; i < s_Data.MeshVertexBufferLayout.GetElements().size(); ++i)
    {
        const auto& CurrentBufferElement = s_Data.MeshVertexBufferLayout.GetElements()[i];
        ShaderAttributeDescriptions.emplace_back(Utility::GetShaderAttributeDescription(
            0, Utility::GauntletShaderDataTypeToVulkan(CurrentBufferElement.Type), i, CurrentBufferElement.Offset));
    }
    PipelineSpec.ShaderAttributeDescriptions = ShaderAttributeDescriptions;

    PipelineSpec.DescriptorSetLayouts = {s_Data.MeshDescriptorSetLayout};
    s_Data.MeshPipeline.reset(new VulkanPipeline(PipelineSpec));
    m_Context.AddSwapchainResizeCallback([this] { s_Data.MeshPipeline->Invalidate(); });
    m_Context.AddSwapchainResizeCallback(
        [this]
        {
            s_Data.PostProcessFramebuffer->GetSpec().Width  = m_Context.GetSwapchain()->GetImageExtent().width;
            s_Data.PostProcessFramebuffer->GetSpec().Height = m_Context.GetSwapchain()->GetImageExtent().height;
            s_Data.PostProcessFramebuffer->Invalidate();
        });

    PipelineSpec.PolygonMode = EPolygonMode::POLYGON_MODE_LINE;
    s_Data.MeshWireframePipeline.reset(new VulkanPipeline(PipelineSpec));
    m_Context.AddSwapchainResizeCallback([this] { s_Data.MeshWireframePipeline->Invalidate(); });

    SetupSkybox();

    const VkDeviceSize CameraDataBufferSize = sizeof(CameraDataBuffer);
    s_Data.UniformCameraDataBuffers.resize(FRAMES_IN_FLIGHT);
    s_Data.MappedUniformCameraDataBuffers.resize(FRAMES_IN_FLIGHT);

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        BufferUtils::CreateBuffer(EBufferUsageFlags::UNIFORM_BUFFER, CameraDataBufferSize, s_Data.UniformCameraDataBuffers[i],
                                  VMA_MEMORY_USAGE_CPU_ONLY);

        s_Data.MappedUniformCameraDataBuffers[i] = m_Context.GetAllocator()->Map(s_Data.UniformCameraDataBuffers[i].Allocation);
    }
}

void VulkanRenderer::BeginSceneImpl(const PerspectiveCamera& InCamera)
{
    s_Data.MeshCameraDataBuffer.Projection = InCamera.GetProjectionMatrix();
    s_Data.MeshCameraDataBuffer.View       = InCamera.GetViewMatrix();
    s_Data.MeshCameraDataBuffer.Position   = InCamera.GetPosition();

    s_Data.SkyboxPushConstants.TransformMatrix =
        InCamera.GetProjectionMatrix() *
        glm::mat4(glm::mat3(InCamera.GetViewMatrix()));  // Removing 4th column of view martix which contains translation;
}

void VulkanRenderer::BeginImpl()
{
    Renderer::GetStats().DrawCalls   = 0;
    s_Data.CurrentDescriptorSetIndex = 0;

    s_Data.CurrentCommandBuffer = &m_Context.GetGraphicsCommandPool()->GetCommandBuffer(m_Context.GetSwapchain()->GetCurrentFrameIndex());
    GNT_ASSERT(s_Data.CurrentCommandBuffer, "Failed to retrieve command buffer!");

    s_Data.CurrentCommandBuffer->BeginDebugLabel("3D", glm::vec4(0.5f, 0.0f, 0.0f, 1.0f));
    s_Data.PostProcessFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());
}

void VulkanRenderer::SubmitMeshImpl(const Ref<Mesh>& InMesh, const glm::mat4& InTransformMatrix)
{
    const auto& Swapchain = m_Context.GetSwapchain();
    GNT_ASSERT(s_Data.CurrentCommandBuffer, "Failed to retrieve command buffer!");

    s_Data.CurrentCommandBuffer->BindPipeline(Renderer::GetSettings().ShowWireframes ? s_Data.MeshWireframePipeline : s_Data.MeshPipeline);

    MeshPushConstants PushConstants = {};
    PushConstants.TransformMatrix   = InTransformMatrix;

    s_Data.CurrentCommandBuffer->BindPushConstants(s_Data.MeshPipeline->GetLayout(),
                                                   s_Data.MeshPipeline->GetPushConstantsShaderStageFlags(), 0,
                                                   s_Data.MeshPipeline->GetPushConstantsSize(), &PushConstants);

    for (uint32_t i = 0; i < InMesh->GetVertexBuffers().size(); ++i)
    {
        if (s_Data.CurrentDescriptorSetIndex >= s_Data.MeshDescriptorSets.size())
        {
            VkDescriptorSet MeshDescriptorSet = VK_NULL_HANDLE;
            GNT_ASSERT(m_Context.GetDescriptorAllocator()->Allocate(&MeshDescriptorSet, s_Data.MeshDescriptorSetLayout),
                       "Failed to allocate descriptor sets!");
            s_Data.MeshDescriptorSets.push_back(MeshDescriptorSet);
        }

        if (!InMesh->IsRendered())
        {
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

            if (InMesh->HasNormalMapTexture(i))
            {
                auto NormalMapImageInfo =
                    std::static_pointer_cast<VulkanTexture2D>(InMesh->GetNormalMapTexture(i))->GetImageDescriptorInfo();
                const auto NormalTextureWriteSet =
                    Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                                                   s_Data.MeshDescriptorSets[s_Data.CurrentDescriptorSetIndex], 1, &NormalMapImageInfo);

                Writes.push_back(NormalTextureWriteSet);
            }
            else
            {
                auto WhiteImageInfo = s_Data.MeshWhiteTexture->GetImageDescriptorInfo();
                const auto WhiteTextureWriteSet =
                    Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                                                   s_Data.MeshDescriptorSets[s_Data.CurrentDescriptorSetIndex], 1, &WhiteImageInfo);

                Writes.push_back(WhiteTextureWriteSet);
            }

            if (InMesh->HasEmissiveTexture(i))
            {
                auto EmissiveImageInfo = std::static_pointer_cast<VulkanTexture2D>(InMesh->GetEmissiveTexture(i))->GetImageDescriptorInfo();
                const auto EmissiveTextureWriteSet =
                    Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2,
                                                   s_Data.MeshDescriptorSets[s_Data.CurrentDescriptorSetIndex], 1, &EmissiveImageInfo);

                Writes.push_back(EmissiveTextureWriteSet);
            }
            else
            {
                auto WhiteImageInfo = s_Data.MeshWhiteTexture->GetImageDescriptorInfo();
                const auto WhiteTextureWriteSet =
                    Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2,
                                                   s_Data.MeshDescriptorSets[s_Data.CurrentDescriptorSetIndex], 1, &WhiteImageInfo);

                Writes.push_back(WhiteTextureWriteSet);
            }

            // Environment Map Texture
            if (auto CubeMapTexture = std::static_pointer_cast<VulkanTextureCube>(s_Data.DefaultSkybox->GetCubeMapTexture()))
            {
                auto CubeMapTextureImageInfo      = CubeMapTexture->GetImage()->GetDescriptorInfo();
                const auto CubeMapTextureWriteSet = Utility::GetWriteDescriptorSet(
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, s_Data.MeshDescriptorSets[s_Data.CurrentDescriptorSetIndex], 1,
                    &CubeMapTextureImageInfo);

                Writes.push_back(CubeMapTextureWriteSet);
            }
            else
            {
                auto WhiteImageInfo = s_Data.MeshWhiteTexture->GetImageDescriptorInfo();
                const auto WhiteTextureWriteSet =
                    Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3,
                                                   s_Data.MeshDescriptorSets[s_Data.CurrentDescriptorSetIndex], 1, &WhiteImageInfo);

                Writes.push_back(WhiteTextureWriteSet);
            }

            VkDescriptorBufferInfo CameraDataBufferInfo = {};
            CameraDataBufferInfo.buffer                 = s_Data.UniformCameraDataBuffers[Swapchain->GetCurrentFrameIndex()].Buffer;
            CameraDataBufferInfo.range                  = sizeof(CameraDataBuffer);
            CameraDataBufferInfo.offset                 = 0;

            // CameraWrite
            auto CameraDataBufferWriteSet =
                Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4,
                                               s_Data.MeshDescriptorSets[s_Data.CurrentDescriptorSetIndex], 1, &CameraDataBufferInfo);

            Writes.push_back(CameraDataBufferWriteSet);

            vkUpdateDescriptorSets(m_Context.GetDevice()->GetLogicalDevice(), static_cast<uint32_t>(Writes.size()), Writes.data(), 0,
                                   VK_NULL_HANDLE);
        }
        memcpy(s_Data.MappedUniformCameraDataBuffers[Swapchain->GetCurrentFrameIndex()], &s_Data.MeshCameraDataBuffer,
               sizeof(CameraDataBuffer));

        s_Data.CurrentCommandBuffer->BindDescriptorSets(s_Data.MeshPipeline->GetLayout(), 0, 1,
                                                        &s_Data.MeshDescriptorSets[s_Data.CurrentDescriptorSetIndex]);
        ++s_Data.CurrentDescriptorSetIndex;

        VkDeviceSize Offset = 0;
        s_Data.CurrentCommandBuffer->BindVertexBuffers(
            0, 1, &std::static_pointer_cast<VulkanVertexBuffer>(InMesh->GetVertexBuffers()[i])->Get(), &Offset);

        s_Data.CurrentCommandBuffer->BindIndexBuffer(std::static_pointer_cast<VulkanIndexBuffer>(InMesh->GetIndexBuffers()[i])->Get(), 0,
                                                     VK_INDEX_TYPE_UINT32);
        s_Data.CurrentCommandBuffer->DrawIndexed(std::static_pointer_cast<VulkanIndexBuffer>(InMesh->GetIndexBuffers()[i])->GetCount());
        ++Renderer::GetStats().DrawCalls;
    }
    InMesh->SetIsRendered(true);
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
             "Failed to create skybox descriptor set layout!");

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
    PipelineSpec.bDepthWrite =
        VK_FALSE;  // I think there's no need to write depth in framebuffer because skybox rendered in the end of all passes
    PipelineSpec.DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    PipelineSpec.TargetFramebuffer = s_Data.PostProcessFramebuffer /*m_Context.GetMainRenderPass()*/;
    PipelineSpec.ShaderStages      = {
        {s_Data.SkyboxVertexShader, EShaderStage::SHADER_STAGE_VERTEX},     //
        {s_Data.SkyboxFragmentShader, EShaderStage::SHADER_STAGE_FRAGMENT}  //
    };

    PipelineSpec.PushConstantRanges        = {Utility::GetPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(MeshPushConstants))};
    PipelineSpec.ShaderBindingDescriptions = {Utility::GetShaderBindingDescription(0, s_Data.SkyboxVertexBufferLayout.GetStride())};

    std::vector<VkVertexInputAttributeDescription> ShaderAttributeDescriptions;
    for (uint32_t i = 0; i < s_Data.SkyboxVertexBufferLayout.GetElements().size(); ++i)
    {
        const auto& CurrentBufferElement = s_Data.SkyboxVertexBufferLayout.GetElements()[i];
        ShaderAttributeDescriptions.emplace_back(Utility::GetShaderAttributeDescription(
            0, Utility::GauntletShaderDataTypeToVulkan(CurrentBufferElement.Type), i, CurrentBufferElement.Offset));
    }
    PipelineSpec.ShaderAttributeDescriptions = ShaderAttributeDescriptions;

    PipelineSpec.DescriptorSetLayouts = {s_Data.SkyboxDescriptorSetLayout};
    s_Data.SkyboxPipeline.reset(new VulkanPipeline(PipelineSpec));
    m_Context.AddSwapchainResizeCallback([this] { s_Data.SkyboxPipeline->Invalidate(); });

    GNT_ASSERT(m_Context.GetDescriptorAllocator()->Allocate(&s_Data.SkyboxDescriptorSet, s_Data.SkyboxDescriptorSetLayout),
               "Failed to allocate descriptor sets!");

    VkWriteDescriptorSet SkyboxWriteDescriptorSet = {};
    auto CubeMapTexture                           = std::static_pointer_cast<VulkanTextureCube>(s_Data.DefaultSkybox->GetCubeMapTexture());
    GNT_ASSERT(CubeMapTexture, "Skybox image is not valid!");

    auto CubeMapTextureImageInfo = CubeMapTexture->GetImage()->GetDescriptorInfo();
    SkyboxWriteDescriptorSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, s_Data.SkyboxDescriptorSet, 1,
                                                              &CubeMapTextureImageInfo);
    vkUpdateDescriptorSets(m_Context.GetDevice()->GetLogicalDevice(), 1, &SkyboxWriteDescriptorSet, 0, VK_NULL_HANDLE);
}

void VulkanRenderer::DrawSkybox()
{
    const auto& Swapchain = m_Context.GetSwapchain();
    GNT_ASSERT(s_Data.CurrentCommandBuffer, "You can't render skybox before you've acquired valid command buffer!");
    s_Data.CurrentCommandBuffer->BeginDebugLabel("Skybox", glm::vec4(0.0f, 0.0f, 0.25f, 1.0f));

    s_Data.CurrentCommandBuffer->BindPipeline(s_Data.SkyboxPipeline);

    static constexpr uint32_t s_MaxSkyboxSize = 460.0f;
    s_Data.SkyboxPushConstants.TransformMatrix *= glm::scale(glm::mat4(1.0f), glm::vec3(s_MaxSkyboxSize));

    s_Data.CurrentCommandBuffer->BindPushConstants(s_Data.SkyboxPipeline->GetLayout(),
                                                   s_Data.SkyboxPipeline->GetPushConstantsShaderStageFlags(), 0,
                                                   s_Data.SkyboxPipeline->GetPushConstantsSize(), &s_Data.SkyboxPushConstants);

    const auto& CubeMesh = s_Data.DefaultSkybox->GetCubeMesh();
    s_Data.CurrentCommandBuffer->BindDescriptorSets(s_Data.SkyboxPipeline->GetLayout(), 0, 1, &s_Data.SkyboxDescriptorSet);

    VkDeviceSize Offset = 0;
    s_Data.CurrentCommandBuffer->BindVertexBuffers(
        0, 1, &std::static_pointer_cast<VulkanVertexBuffer>(CubeMesh->GetVertexBuffers()[0])->Get(), &Offset);

    s_Data.CurrentCommandBuffer->BindIndexBuffer(std::static_pointer_cast<VulkanIndexBuffer>(CubeMesh->GetIndexBuffers()[0])->Get(), 0,
                                                 VK_INDEX_TYPE_UINT32);
    s_Data.CurrentCommandBuffer->DrawIndexed(std::static_pointer_cast<VulkanIndexBuffer>(CubeMesh->GetIndexBuffers()[0])->GetCount());

    s_Data.CurrentCommandBuffer->EndDebugLabel();
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

    s_Data.PostProcessFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
    s_Data.CurrentCommandBuffer->EndDebugLabel();
}

const Ref<VulkanFramebuffer>& VulkanRenderer::GetPostProcessFramebuffer()
{
    return s_Data.PostProcessFramebuffer;
}

const VkDescriptorSetLayout& VulkanRenderer::GetImageDescriptorSetLayout()
{
    return s_Data.ImageDescriptorSetLayout;
}

const Ref<Image> VulkanRenderer::GetFinalImageImpl()
{
    return GetPostProcessFramebuffer()->GetColorAttachments()[m_Context.GetSwapchain()->GetCurrentImageIndex()];
}

void VulkanRenderer::Destroy()
{
    m_Context.GetDevice()->WaitDeviceOnFinish();

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        m_Context.GetAllocator()->Unmap(s_Data.UniformCameraDataBuffers[i].Allocation);
        BufferUtils::DestroyBuffer(s_Data.UniformCameraDataBuffers[i]);
    }

    s_Data.PostProcessFramebuffer->Destroy();
    vkDestroyDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), s_Data.MeshDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), s_Data.ImageDescriptorSetLayout, nullptr);

    s_Data.MeshWireframePipeline->Destroy();
    s_Data.MeshPipeline->Destroy();
    s_Data.MeshVertexShader->DestroyModule();
    s_Data.MeshFragmentShader->DestroyModule();
    s_Data.MeshWhiteTexture->Destroy();
    s_Data.CurrentCommandBuffer = nullptr;

    DestroySkybox();
}

}  // namespace Gauntlet