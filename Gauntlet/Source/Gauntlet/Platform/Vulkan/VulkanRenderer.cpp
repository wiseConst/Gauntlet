#include "GauntletPCH.h"
#include "VulkanRenderer.h"

#include "Gauntlet/Renderer/Camera/PerspectiveCamera.h"
#include "Gauntlet/Core/JobSystem.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanPipeline.h"
#include "VulkanFramebuffer.h"

#include "VulkanRenderer2D.h"
#include "VulkanUtility.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanTextureCube.h"
#include "VulkanBuffer.h"
#include "VulkanMaterial.h"

#include "Gauntlet/Renderer/Skybox.h"
#include "Gauntlet/Renderer/Mesh.h"

namespace Gauntlet
{

VulkanRenderer::VulkanRenderer() : m_Context((VulkanContext&)VulkanContext::Get())
{
    Create();
}

void VulkanRenderer::Create()
{
    {
        const auto TextureDescriptorSetLayoutBinding =
            Utility::GetDescriptorSetLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        VkDescriptorSetLayoutCreateInfo ImageDescriptorSetLayoutCreateInfo =
            Utility::GetDescriptorSetLayoutCreateInfo(1, &TextureDescriptorSetLayoutBinding);
        VK_CHECK(vkCreateDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), &ImageDescriptorSetLayoutCreateInfo, nullptr,
                                             &s_Data.ImageDescriptorSetLayout),
                 "Failed to create texture(UI) descriptor set layout!");

        const uint32_t WhiteTexutreData = 0xffffffff;
        s_Data.MeshWhiteTexture         = MakeRef<VulkanTexture2D>(&WhiteTexutreData, sizeof(WhiteTexutreData), 1, 1);
    }

    // Geometry pipeline
    {
        FramebufferSpecification geometryFramebufferSpec = {};

        FramebufferAttachmentSpecification attachment = {};
        attachment.LoadOp                             = ELoadOp::LOAD;
        attachment.StoreOp                            = EStoreOp::STORE;
        attachment.ClearColor                         = glm::vec4(glm::vec3(0.1f), 1.0f);

        // Position
        attachment.Format = EImageFormat::RGBA16F;
        geometryFramebufferSpec.Attachments.push_back(attachment);

        // Normal
        attachment.Format = EImageFormat::RGBA16F;
        geometryFramebufferSpec.Attachments.push_back(attachment);

        // Albedo
        attachment.Format = EImageFormat::RGBA;
        geometryFramebufferSpec.Attachments.push_back(attachment);

        // Depth
        attachment.ClearColor = glm::vec4(1.0f, glm::vec3(0.0f));
        attachment.Format     = EImageFormat::DEPTH32F;
        geometryFramebufferSpec.Attachments.push_back(attachment);

        geometryFramebufferSpec.Name = "Geometry";

        s_Data.GeometryFramebuffer = MakeRef<VulkanFramebuffer>(geometryFramebufferSpec);

        auto GeometryShader = std::static_pointer_cast<VulkanShader>(ShaderLibrary::Load("Resources/Cached/Shaders/Geometry"));
        s_Data.GeometryDescriptorSetLayout       = GeometryShader->GetDescriptorSetLayouts()[0];
        s_RendererStorage.MeshVertexBufferLayout = GeometryShader->GetVertexBufferLayout();

        PipelineSpecification geometryPipelineSpec = {};
        geometryPipelineSpec.Name                  = "Geometry";
        geometryPipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        geometryPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        geometryPipelineSpec.CullMode              = ECullMode::CULL_MODE_BACK;
        geometryPipelineSpec.bDepthWrite           = VK_TRUE;
        geometryPipelineSpec.bDepthTest            = VK_TRUE;
        geometryPipelineSpec.DepthCompareOp        = VK_COMPARE_OP_LESS;
        geometryPipelineSpec.TargetFramebuffer     = s_Data.GeometryFramebuffer;
        geometryPipelineSpec.Shader                = GeometryShader;
        geometryPipelineSpec.bDynamicPolygonMode   = VK_TRUE;

        s_Data.GeometryPipeline = MakeRef<VulkanPipeline>(geometryPipelineSpec);
    }

    {
        FramebufferSpecification setupFramebufferSpec = {};
        setupFramebufferSpec.ExistingAttachments      = s_Data.GeometryFramebuffer->GetAttachments();
        setupFramebufferSpec.Name                     = "Setup";

        s_Data.SetupFramebuffer = MakeRef<VulkanFramebuffer>(setupFramebufferSpec);
    }

    // Forward
    {

        PipelineSpecification forwardPipelineSpec = {};
        forwardPipelineSpec.Name                  = "ForwardPass";
        forwardPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        forwardPipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        forwardPipelineSpec.PrimitiveTopology     = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        forwardPipelineSpec.CullMode              = ECullMode::CULL_MODE_NONE;
        forwardPipelineSpec.bDepthTest            = VK_TRUE;
        forwardPipelineSpec.bDepthWrite           = VK_TRUE;
        forwardPipelineSpec.DepthCompareOp        = VK_COMPARE_OP_LESS;
        forwardPipelineSpec.TargetFramebuffer     = s_Data.GeometryFramebuffer;

        auto MeshShader = std::static_pointer_cast<VulkanShader>(ShaderLibrary::Load("Resources/Cached/Shaders/Forward"));
        s_RendererStorage.MeshVertexBufferLayout = MeshShader->GetVertexBufferLayout();
        s_Data.MeshDescriptorSetLayout           = MeshShader->GetDescriptorSetLayouts()[0];

        forwardPipelineSpec.Shader = MeshShader;

        s_Data.ForwardPassPipeline = MakeRef<VulkanPipeline>(forwardPipelineSpec);
    }

    SetupSkybox();

    constexpr VkDeviceSize CameraDataBufferSize = sizeof(CameraDataBuffer);
    s_Data.UniformCameraDataBuffers.resize(FRAMES_IN_FLIGHT);
    s_Data.MappedUniformCameraDataBuffers.resize(FRAMES_IN_FLIGHT);

    constexpr VkDeviceSize PhongModelBufferSize = sizeof(LightingModelBuffer);
    s_Data.UniformPhongModelBuffers.resize(FRAMES_IN_FLIGHT);
    s_Data.MappedUniformPhongModelBuffers.resize(FRAMES_IN_FLIGHT);

    constexpr VkDeviceSize ShadowsBufferBufferSize = sizeof(ShadowsBuffer);
    s_Data.UniformShadowsBuffers.resize(FRAMES_IN_FLIGHT);
    s_Data.MappedUniformShadowsBuffers.resize(FRAMES_IN_FLIGHT);
    for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
    {
        // Camera
        BufferUtils::CreateBuffer(EBufferUsageFlags::UNIFORM_BUFFER, CameraDataBufferSize, s_Data.UniformCameraDataBuffers[frame],
                                  VMA_MEMORY_USAGE_CPU_ONLY);

        s_Data.MappedUniformCameraDataBuffers[frame] = m_Context.GetAllocator()->Map(s_Data.UniformCameraDataBuffers[frame].Allocation);

        // Lighting
        BufferUtils::CreateBuffer(EBufferUsageFlags::UNIFORM_BUFFER, PhongModelBufferSize, s_Data.UniformPhongModelBuffers[frame],
                                  VMA_MEMORY_USAGE_CPU_ONLY);

        s_Data.MappedUniformPhongModelBuffers[frame] = m_Context.GetAllocator()->Map(s_Data.UniformPhongModelBuffers[frame].Allocation);

        // Shadows
        BufferUtils::CreateBuffer(EBufferUsageFlags::UNIFORM_BUFFER, ShadowsBufferBufferSize, s_Data.UniformShadowsBuffers[frame],
                                  VMA_MEMORY_USAGE_CPU_ONLY);

        s_Data.MappedUniformShadowsBuffers[frame] = m_Context.GetAllocator()->Map(s_Data.UniformShadowsBuffers[frame].Allocation);
    }

    // ShadowMap
    {
        FramebufferSpecification shadowMapFramebufferSpec = {};

        FramebufferAttachmentSpecification shadowMapAttachment = {};
        shadowMapAttachment.ClearColor                         = glm::vec4(1.0f, glm::vec3(0.0f));
        shadowMapAttachment.Format                             = EImageFormat::DEPTH32F;
        shadowMapAttachment.LoadOp                             = ELoadOp::CLEAR;
        shadowMapAttachment.StoreOp                            = EStoreOp::STORE;
        shadowMapAttachment.Filter                             = ETextureFilter::NEAREST;
        shadowMapAttachment.Wrap                               = ETextureWrap::CLAMP;

        shadowMapFramebufferSpec.Attachments = {shadowMapAttachment};
        shadowMapFramebufferSpec.Name        = "ShadowMap";
        shadowMapFramebufferSpec.Width       = 4096;
        shadowMapFramebufferSpec.Height      = 4096;

        s_Data.ShadowMapFramebuffer = MakeRef<VulkanFramebuffer>(shadowMapFramebufferSpec);

        PipelineSpecification geometryPipelineSpec = {};
        geometryPipelineSpec.Name                  = "ShadowMap";
        geometryPipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        geometryPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        geometryPipelineSpec.CullMode              = ECullMode::CULL_MODE_BACK;
        geometryPipelineSpec.bDepthWrite           = VK_TRUE;
        geometryPipelineSpec.bDepthTest            = VK_TRUE;
        geometryPipelineSpec.DepthCompareOp        = VK_COMPARE_OP_LESS;
        geometryPipelineSpec.TargetFramebuffer     = s_Data.ShadowMapFramebuffer;
        geometryPipelineSpec.Layout                = Renderer::GetStorageData().MeshVertexBufferLayout;
        geometryPipelineSpec.Shader = std::static_pointer_cast<VulkanShader>(ShaderLibrary::Load("Resources/Cached/Shaders/Depth"));

        s_Data.ShadowMapPipeline = MakeRef<VulkanPipeline>(geometryPipelineSpec);
    }

    // SSAO
    {
        /* FramebufferSpecification ssaoFramebufferSpec = {};

         s_Data.SSAOFramebuffer = MakeRef<VulkanFramebuffer>(ssaoFramebufferSpec);

         PipelineSpecification ssaoPipelineSpec = {};

         s_Data.SSAOPipeline = MakeRef<VulkanPipeline>(ssaoPipelineSpec);*/
    }

    // Lighting
    {
        FramebufferSpecification lightingFramebufferSpec = {};

        FramebufferAttachmentSpecification attachment = {};
        attachment.ClearColor                         = glm::vec4(glm::vec3(0.1f), 1.0f);
        attachment.Format                             = EImageFormat::RGBA;
        // attachment.LoadOp     = ELoadOp::LOAD;
        attachment.StoreOp = EStoreOp::STORE;
        lightingFramebufferSpec.Attachments.push_back(attachment);

        lightingFramebufferSpec.Name = "FinalPass";
        s_Data.LightingFramebuffer   = MakeRef<VulkanFramebuffer>(lightingFramebufferSpec);

        auto LightingShader = std::static_pointer_cast<VulkanShader>(ShaderLibrary::Load("Resources/Cached/Shaders/Lighting"));
        s_Data.LightingDescriptorSetLayout = LightingShader->GetDescriptorSetLayouts()[0];

        GNT_ASSERT(m_Context.GetDescriptorAllocator()->Allocate(s_Data.LightingSet, s_Data.LightingDescriptorSetLayout),
                   "Failed to allocate lighting descriptor set!");

        for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            std::vector<VkWriteDescriptorSet> writes;

            for (uint32_t k = 0; k < s_Data.GeometryFramebuffer->GetAttachments().size() - 1; ++k)
            {
                Ref<VulkanImage> vulkanImage =
                    static_pointer_cast<VulkanImage>(s_Data.GeometryFramebuffer->GetAttachments()[k].Attachments[frame]);
                auto textureWriteSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, k,
                                                                      s_Data.LightingSet.Handle, 1, &vulkanImage->GetDescriptorInfo());
                writes.push_back(textureWriteSet);
            }

            VkDescriptorBufferInfo LightingModelBufferInfo = {};
            LightingModelBufferInfo.buffer                 = s_Data.UniformPhongModelBuffers[frame].Buffer;
            LightingModelBufferInfo.range                  = sizeof(LightingModelBuffer);
            LightingModelBufferInfo.offset                 = 0;

            // PhongModel
            auto PhongModelBufferWriteSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3, s_Data.LightingSet.Handle,
                                                                           1, &LightingModelBufferInfo);

            writes.push_back(PhongModelBufferWriteSet);

            vkUpdateDescriptorSets(m_Context.GetDevice()->GetLogicalDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0,
                                   nullptr);
        }

        PipelineSpecification lightingPipelineSpec = {};
        lightingPipelineSpec.Name                  = "Lighting";
        lightingPipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        lightingPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        lightingPipelineSpec.TargetFramebuffer     = s_Data.LightingFramebuffer;
        lightingPipelineSpec.Shader                = LightingShader;

        s_Data.LightingPipeline = MakeRef<VulkanPipeline>(lightingPipelineSpec);
    }
}

void VulkanRenderer::BeginSceneImpl(const PerspectiveCamera& camera)
{
    s_Data.MeshCameraDataBuffer.Projection = camera.GetProjectionMatrix();
    s_Data.MeshCameraDataBuffer.View       = camera.GetViewMatrix();
    s_Data.MeshCameraDataBuffer.Position   = camera.GetPosition();

    memcpy(s_Data.MappedUniformCameraDataBuffers[m_Context.GetSwapchain()->GetCurrentFrameIndex()], &s_Data.MeshCameraDataBuffer,
           sizeof(CameraDataBuffer));

    VulkanRenderer2D::GetStorageData().CameraProjectionMatrix = camera.GetViewProjectionMatrix();
    s_Data.SkyboxPushConstants.TransformMatrix =
        camera.GetProjectionMatrix() *
        glm::mat4(glm::mat3(camera.GetViewMatrix()));  // Removing 4th column of view martix which contains translation;
}

void VulkanRenderer::BeginImpl()
{
    s_Data.CurrentCommandBuffer = &m_Context.GetGraphicsCommandPool()->GetCommandBuffer(m_Context.GetSwapchain()->GetCurrentFrameIndex());
    GNT_ASSERT(s_Data.CurrentCommandBuffer, "Failed to retrieve command buffer!");

    Renderer::GetStats().DrawCalls = 0;

    if (s_Data.bFramebuffersNeedResize)
    {
        m_Context.GetDevice()->WaitDeviceOnFinish();
        s_Data.GeometryFramebuffer->Resize(s_Data.NewFramebufferSize.x, s_Data.NewFramebufferSize.y);
        s_Data.SetupFramebuffer->Resize(s_Data.NewFramebufferSize.x, s_Data.NewFramebufferSize.y);

        /*for (auto& geometry : s_Data.SortedGeometry)
        {
            geometry.Material->Invalidate();
        }*/

        for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            std::vector<VkWriteDescriptorSet> writes;

            for (uint32_t k = 0; k < s_Data.GeometryFramebuffer->GetAttachments().size() - 1; ++k)
            {
                Ref<VulkanImage> vulkanImage =
                    static_pointer_cast<VulkanImage>(s_Data.GeometryFramebuffer->GetAttachments()[k].Attachments[frame]);
                auto textureWriteSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, k,
                                                                      s_Data.LightingSet.Handle, 1, &vulkanImage->GetDescriptorInfo());
                writes.push_back(textureWriteSet);
            }

            VkDescriptorBufferInfo LightingModelBufferInfo = {};
            LightingModelBufferInfo.buffer                 = s_Data.UniformPhongModelBuffers[frame].Buffer;
            LightingModelBufferInfo.range                  = sizeof(LightingModelBuffer);
            LightingModelBufferInfo.offset                 = 0;

            // PhongModel
            auto PhongModelBufferWriteSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3, s_Data.LightingSet.Handle,
                                                                           1, &LightingModelBufferInfo);

            writes.push_back(PhongModelBufferWriteSet);

            vkUpdateDescriptorSets(m_Context.GetDevice()->GetLogicalDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0,
                                   nullptr);
        }

        s_Data.bFramebuffersNeedResize = false;
    }

    // Clear geometry buffer contents
    s_Data.CurrentCommandBuffer->BeginDebugLabel("SetupPass", glm::vec4(0.3f, 0.56f, 0.841f, 1.0f));
    s_Data.SetupFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());

    s_Data.SetupFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
    s_Data.CurrentCommandBuffer->EndDebugLabel();

    {
        s_Data.MeshLightingModelBuffer.Gamma = s_RendererSettings.Gamma;
        // Reset all the light sources to default values in case we deleted them, because in the next frame they will be added to buffer
        // if they weren't removed.
        s_Data.MeshLightingModelBuffer.DirLight = DirectionalLight();

        for (uint32_t i = 0; i < s_Data.CurrentPointLightIndex; ++i)
            s_Data.MeshLightingModelBuffer.PointLights[i] = PointLight();

        memcpy(s_Data.MappedUniformPhongModelBuffers[m_Context.GetSwapchain()->GetCurrentFrameIndex()], &s_Data.MeshLightingModelBuffer,
               sizeof(LightingModelBuffer));
    }
    s_Data.CurrentPointLightIndex = 0;

    s_Data.SortedGeometry.clear();
}

void VulkanRenderer::SubmitMeshImpl(const Ref<Mesh>& mesh, const glm::mat4& transform)
{
    for (uint32_t i = 0; i < mesh->GetSubmeshCount(); ++i)
    {
        const auto vulkanMaterial = static_pointer_cast<Gauntlet::VulkanMaterial>(mesh->GetMaterial(i));
        GNT_ASSERT(vulkanMaterial, "Failed to retrieve mesh material!");

        Ref<Gauntlet::VulkanVertexBuffer> vulkanVertexBuffer =
            std::static_pointer_cast<Gauntlet::VulkanVertexBuffer>(mesh->GetVertexBuffers()[i]);

        Ref<Gauntlet::VulkanIndexBuffer> vulkanIndexBuffer =
            std::static_pointer_cast<Gauntlet::VulkanIndexBuffer>(mesh->GetIndexBuffers()[i]);

        s_Data.SortedGeometry.emplace_back(vulkanMaterial, vulkanVertexBuffer, vulkanIndexBuffer, transform);
    }
}

void VulkanRenderer::AddPointLightImpl(const glm::vec3& position, const glm::vec3& color, const glm::vec3& AmbientSpecularShininess,
                                       const glm::vec3& CLQ)
{
    if (s_Data.CurrentPointLightIndex >= s_MAX_POINT_LIGHTS) return;

    s_Data.MeshLightingModelBuffer.PointLights[s_Data.CurrentPointLightIndex].Position = glm::vec4(position, 0.0f);
    s_Data.MeshLightingModelBuffer.PointLights[s_Data.CurrentPointLightIndex].Color    = glm::vec4(color, 0.0f);
    s_Data.MeshLightingModelBuffer.PointLights[s_Data.CurrentPointLightIndex].AmbientSpecularShininess =
        glm::vec4(AmbientSpecularShininess, 0.0f);
    s_Data.MeshLightingModelBuffer.PointLights[s_Data.CurrentPointLightIndex].CLQ = glm::vec4(CLQ, 0.0f);

    ++s_Data.CurrentPointLightIndex;
}

void VulkanRenderer::AddDirectionalLightImpl(const glm::vec3& color, const glm::vec3& direction, const glm::vec3& AmbientSpecularShininess)
{
    s_Data.MeshLightingModelBuffer.Gamma = s_RendererSettings.Gamma;

    s_Data.MeshLightingModelBuffer.DirLight.Color                    = glm::vec4(color, 0.0f);
    s_Data.MeshLightingModelBuffer.DirLight.Direction                = glm::vec4(direction, 0.0f);
    s_Data.MeshLightingModelBuffer.DirLight.AmbientSpecularShininess = glm::vec4(AmbientSpecularShininess, 0.0f);
}

void VulkanRenderer::SetupSkybox()
{
    const auto SkyboxPath                = std::string("Resources/Textures/Skybox/Yokohama2/");
    const std::vector<std::string> Faces = {
        SkyboxPath + "right.jpg",   //
        SkyboxPath + "left.jpg",    //
        SkyboxPath + "bottom.jpg",  // Swapped because I flip the viewport's Y axis
        SkyboxPath + "top.jpg",     // Swapped
        SkyboxPath + "front.jpg",   //
        SkyboxPath + "back.jpg"     //
    };
    s_Data.DefaultSkybox = MakeRef<Skybox>(Faces);

    auto SkyboxShader = std::static_pointer_cast<VulkanShader>(ShaderLibrary::Load("Resources/Cached/Shaders/Skybox"));

    s_Data.SkyboxDescriptorSetLayout = SkyboxShader->GetDescriptorSetLayouts()[0];
    s_Data.SkyboxVertexBufferLayout  = SkyboxShader->GetVertexBufferLayout();

    // Base skybox pipeline creation
    PipelineSpecification PipelineSpec = {};
    PipelineSpec.Name                  = "Skybox";
    PipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
    PipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
    PipelineSpec.PrimitiveTopology     = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PipelineSpec.CullMode              = ECullMode::CULL_MODE_BACK;
    PipelineSpec.bDepthTest            = VK_TRUE;
    PipelineSpec.bDepthWrite =
        VK_FALSE;  // I think there's no need to write depth in framebuffer because skybox rendered in the end of all passes
    PipelineSpec.DepthCompareOp = VK_COMPARE_OP_LESS;

    PipelineSpec.TargetFramebuffer = s_Data.GeometryFramebuffer;
    PipelineSpec.Shader            = SkyboxShader;

    s_Data.SkyboxPipeline = MakeRef<VulkanPipeline>(PipelineSpec);

    GNT_ASSERT(m_Context.GetDescriptorAllocator()->Allocate(s_Data.SkyboxDescriptorSet, s_Data.SkyboxDescriptorSetLayout),
               "Failed to allocate descriptor sets!");

    auto CubeMapTexture = std::static_pointer_cast<VulkanTextureCube>(s_Data.DefaultSkybox->GetCubeMapTexture());
    GNT_ASSERT(CubeMapTexture, "Skybox image is not valid!");

    auto CubeMapTextureImageInfo        = CubeMapTexture->GetImage()->GetDescriptorInfo();
    const auto SkyboxWriteDescriptorSet = Utility::GetWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
                                                                         s_Data.SkyboxDescriptorSet.Handle, 1, &CubeMapTextureImageInfo);
    vkUpdateDescriptorSets(m_Context.GetDevice()->GetLogicalDevice(), 1, &SkyboxWriteDescriptorSet, 0, VK_NULL_HANDLE);
}

void VulkanRenderer::DrawSkybox()
{
    const auto& Swapchain = m_Context.GetSwapchain();
    GNT_ASSERT(s_Data.CurrentCommandBuffer, "You can't render skybox before you've acquired valid command buffer!");
    s_Data.CurrentCommandBuffer->BeginDebugLabel("Skybox", glm::vec4(0.0f, 0.0f, 0.25f, 1.0f));

    s_Data.CurrentCommandBuffer->BindPipeline(s_Data.SkyboxPipeline);

    static constexpr uint32_t s_MaxSkyboxSize = 460;
    s_Data.SkyboxPushConstants.TransformMatrix *= glm::scale(glm::mat4(1.0f), glm::vec3(s_MaxSkyboxSize));

    s_Data.CurrentCommandBuffer->BindPushConstants(s_Data.SkyboxPipeline->GetLayout(),
                                                   s_Data.SkyboxPipeline->GetPushConstantsShaderStageFlags(), 0,
                                                   s_Data.SkyboxPipeline->GetPushConstantsSize(), &s_Data.SkyboxPushConstants);

    const auto& CubeMesh = s_Data.DefaultSkybox->GetCubeMesh();
    s_Data.CurrentCommandBuffer->BindDescriptorSets(s_Data.SkyboxPipeline->GetLayout(), 0, 1, &s_Data.SkyboxDescriptorSet.Handle);

    VkDeviceSize Offset = 0;
    s_Data.CurrentCommandBuffer->BindVertexBuffers(
        0, 1, &std::static_pointer_cast<VulkanVertexBuffer>(CubeMesh->GetVertexBuffers()[0])->Get(), &Offset);

    Ref<Gauntlet::VulkanIndexBuffer> VulkanIndexBuffer =
        std::static_pointer_cast<Gauntlet::VulkanIndexBuffer>(CubeMesh->GetIndexBuffers()[0]);
    s_Data.CurrentCommandBuffer->BindIndexBuffer(VulkanIndexBuffer->Get(), 0, VK_INDEX_TYPE_UINT32);
    s_Data.CurrentCommandBuffer->DrawIndexed(static_cast<uint32_t>(VulkanIndexBuffer->GetCount()));

    s_Data.CurrentCommandBuffer->EndDebugLabel();
}

void VulkanRenderer::DestroySkybox()
{
    m_Context.GetDescriptorAllocator()->ReleaseDescriptorSets(&s_Data.SkyboxDescriptorSet, 1);
    s_Data.SkyboxPipeline->Destroy();
    s_Data.DefaultSkybox->Destroy();
}

void VulkanRenderer::EndSceneImpl()
{
    std::sort(s_Data.SortedGeometry.begin(), s_Data.SortedGeometry.end(),
              [&](const GeometryData& lhs, const GeometryData& rhs)
              {
                  const glm::vec3& cameraPos = s_Data.MeshCameraDataBuffer.Position;
                  const glm::vec3 lhsPos(lhs.Transform[3]);
                  const glm::vec3 rhsPos(rhs.Transform[3]);

                  const float lhsLength = glm::length(cameraPos - lhsPos);
                  const float rhsLength = glm::length(cameraPos - rhsPos);

                  return lhsLength > rhsLength;  // Descending order
              });

    GNT_ASSERT(s_Data.CurrentCommandBuffer, "Failed to retrieve command buffer!");

    // Multithreaded stuff

    // Add a job to the thread's queue for each object to be rendered
    const uint32_t threadCount   = JobSystem::GetThreadCount() > 0 ? JobSystem::GetThreadCount() : 1;
    const uint32_t objectsUint32 = (uint32_t)s_Data.SortedGeometry.size() / threadCount;
    bool bAreThereRemainingObjects{false};
    {
        const float objectsFloat  = static_cast<float>(s_Data.SortedGeometry.size()) / threadCount;
        bAreThereRemainingObjects = objectsFloat > static_cast<float>(objectsUint32);
    }

    const uint32_t objectsPerThread = bAreThereRemainingObjects ? objectsUint32 + 1 : objectsUint32;
    auto& threads                   = JobSystem::GetThreads();
    auto& threadData                = m_Context.GetThreadData();

    // ShadowMapPass
    {
        s_Data.ShadowMapFramebuffer->SetDepthStencilClearColor(GetSettings().RenderShadows ? 1.0f : 0.0f, 0);

        s_Data.CurrentCommandBuffer->BeginDebugLabel("ShadowMapPass", glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
        if (!Renderer::GetSettings().RenderShadows)
        {
            s_Data.ShadowMapFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());
        }
        else
        {
            s_Data.ShadowMapFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get(), VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

            constexpr float cameraWidth     = 30.0f;
            constexpr float zNear           = 1.0f;
            constexpr float zFar            = 128.0f;
            const glm::mat4 lightProjection = glm::ortho(-cameraWidth, cameraWidth, -cameraWidth, cameraWidth, zNear, zFar);

            const glm::mat4 lightView =
                glm::lookAt(glm::vec3(s_Data.MeshLightingModelBuffer.DirLight.Direction), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            s_Data.MeshShadowsBuffer.LightSpaceMatrix = lightProjection * lightView;

            LightPushConstants pushConstants   = {};
            pushConstants.LightSpaceProjection = s_Data.MeshShadowsBuffer.LightSpaceMatrix;
            pushConstants.Model                = glm::mat4(1.0f);

            const auto inheritanceInfo =
                Utility::GetCommandBufferInheritanceInfo(s_Data.ShadowMapFramebuffer->GetRenderPass(), s_Data.ShadowMapFramebuffer->Get());

            uint32_t currentGeometryIndex = 0;
            std::vector<VkCommandBuffer> recordedSecondaryShadowMapCommandBuffers;

            for (uint32_t t = 0; t < threadCount; ++t)
            {
                std::vector<GeometryData> objects;
                for (uint32_t i = 0; i < objectsPerThread; ++i)
                {
                    if (currentGeometryIndex >= s_Data.SortedGeometry.size())
                        break;
                    else
                    {
                        objects.push_back(s_Data.SortedGeometry[currentGeometryIndex]);
                        ++currentGeometryIndex;
                    }
                }

                if (objects.empty()) break;

                auto& currentSecondaryCommandBuffer =
                    threadData[t].SecondaryShadowMapCommandBuffers[m_Context.GetSwapchain()->GetCurrentFrameIndex()];
                recordedSecondaryShadowMapCommandBuffers.push_back(currentSecondaryCommandBuffer.Get());
                threads[t].Submit(
                    [=]
                    {
                        currentSecondaryCommandBuffer.BeginRecording(0, &inheritanceInfo);
                        currentSecondaryCommandBuffer.BindPipeline(s_Data.ShadowMapPipeline);

                        for (auto& object : objects)
                        {
                            currentSecondaryCommandBuffer.BindPushConstants(
                                s_Data.ShadowMapPipeline->GetLayout(), s_Data.ShadowMapPipeline->GetPushConstantsShaderStageFlags(), 0,
                                s_Data.ShadowMapPipeline->GetPushConstantsSize(), &pushConstants);

                            VkDeviceSize Offset = 0;
                            currentSecondaryCommandBuffer.BindVertexBuffers(0, 1, &object.VulkanVertexBuffer->Get(), &Offset);

                            currentSecondaryCommandBuffer.BindIndexBuffer(object.VulkanIndexBuffer->Get(), 0, VK_INDEX_TYPE_UINT32);
                            currentSecondaryCommandBuffer.DrawIndexed(static_cast<uint32_t>(object.VulkanIndexBuffer->GetCount()));
                            ++Renderer::GetStats().DrawCalls;
                        }

                        currentSecondaryCommandBuffer.EndRecording();
                    });
            }

            if (!recordedSecondaryShadowMapCommandBuffers.empty())
            {
                JobSystem::Wait();
                vkCmdExecuteCommands(s_Data.CurrentCommandBuffer->Get(),
                                     static_cast<uint32_t>(recordedSecondaryShadowMapCommandBuffers.size()),
                                     recordedSecondaryShadowMapCommandBuffers.data());
            }
        }

        s_Data.ShadowMapFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
        s_Data.CurrentCommandBuffer->EndDebugLabel();
    }

    // Actual GeometryPass
    {
        s_Data.CurrentCommandBuffer->BeginDebugLabel("GeometryPass", glm::vec4(0.5f, 0.0f, 0.0f, 1.0f));

        s_Data.GeometryFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get(), VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

        //// Set shadow buffer(only contains light space mvp mat4)
        // memcpy(s_Data.MappedUniformShadowsBuffers[m_Context.GetSwapchain()->GetCurrentFrameIndex()], &s_Data.MeshShadowsBuffer,
        //        sizeof(ShadowsBuffer));

        // Set lighting stuff
        memcpy(s_Data.MappedUniformPhongModelBuffers[m_Context.GetSwapchain()->GetCurrentFrameIndex()], &s_Data.MeshLightingModelBuffer,
               sizeof(LightingModelBuffer));

        const auto inheritanceInfo =
            Utility::GetCommandBufferInheritanceInfo(s_Data.GeometryFramebuffer->GetRenderPass(), s_Data.GeometryFramebuffer->Get());

        // TODO: Handle while there's no multithreading.
        uint32_t currentGeometryIndex = 0;
        std::vector<VkCommandBuffer> recordedSecondaryGeometryCommandBuffers;

        for (uint32_t t = 0; t < threadCount; ++t)
        {
            std::vector<GeometryData> objects;
            for (uint32_t i = 0; i < objectsPerThread; ++i)
            {
                if (currentGeometryIndex >= s_Data.SortedGeometry.size())
                    break;
                else
                {
                    objects.push_back(s_Data.SortedGeometry[currentGeometryIndex]);
                    ++currentGeometryIndex;
                }
            }

            if (objects.empty()) break;

            auto& currentSecondaryCommandBuffer =
                threadData[t].SecondaryGeometryCommandBuffers[m_Context.GetSwapchain()->GetCurrentFrameIndex()];
            recordedSecondaryGeometryCommandBuffers.push_back(currentSecondaryCommandBuffer.Get());
            threads[t].Submit(
                [=]
                {
                    currentSecondaryCommandBuffer.BeginRecording(0, &inheritanceInfo);
                    currentSecondaryCommandBuffer.SetPipelinePolygonMode(s_Data.GeometryPipeline, GetSettings().ShowWireframes
                                                                                                      ? EPolygonMode::POLYGON_MODE_LINE
                                                                                                      : EPolygonMode::POLYGON_MODE_FILL);
                    currentSecondaryCommandBuffer.BindPipeline(s_Data.GeometryPipeline);

                    for (auto& object : objects)
                    {
                        MeshPushConstants PushConstants = {};
                        PushConstants.TransformMatrix   = object.Transform;

                        currentSecondaryCommandBuffer.BindPushConstants(s_Data.GeometryPipeline->GetLayout(),
                                                                        s_Data.GeometryPipeline->GetPushConstantsShaderStageFlags(), 0,
                                                                        s_Data.GeometryPipeline->GetPushConstantsSize(), &PushConstants);

                        auto MaterialDescriptorSet = object.Material->GetDescriptorSet();
                        currentSecondaryCommandBuffer.BindDescriptorSets(s_Data.GeometryPipeline->GetLayout(), 0, 1,
                                                                         &MaterialDescriptorSet);

                        VkDeviceSize Offset = 0;
                        currentSecondaryCommandBuffer.BindVertexBuffers(0, 1, &object.VulkanVertexBuffer->Get(), &Offset);

                        currentSecondaryCommandBuffer.BindIndexBuffer(object.VulkanIndexBuffer->Get(), 0, VK_INDEX_TYPE_UINT32);
                        currentSecondaryCommandBuffer.DrawIndexed(static_cast<uint32_t>(object.VulkanIndexBuffer->GetCount()));
                        ++Renderer::GetStats().DrawCalls;
                    }

                    currentSecondaryCommandBuffer.EndRecording();
                });
        }

        if (!recordedSecondaryGeometryCommandBuffers.empty())
        {
            JobSystem::Wait();
            vkCmdExecuteCommands(s_Data.CurrentCommandBuffer->Get(), static_cast<uint32_t>(recordedSecondaryGeometryCommandBuffers.size()),
                                 recordedSecondaryGeometryCommandBuffers.data());
        }

        s_Data.GeometryFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
        s_Data.CurrentCommandBuffer->EndDebugLabel();
    }

    // Render skybox as last geometry to prevent depth testing as mush as possible
    s_Data.CurrentCommandBuffer->BeginDebugLabel("Skybox", glm::vec4(0.5f, 0.0f, 0.0f, 1.0f));
    s_Data.GeometryFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());

    DrawSkybox();

    s_Data.GeometryFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
    s_Data.CurrentCommandBuffer->EndDebugLabel();
}

void VulkanRenderer::FlushImpl()
{
    s_Data.CurrentCommandBuffer->BeginDebugLabel("LightingPass", glm::vec4(0.7f, 0.7f, 0.0f, 1.0f));
    s_Data.LightingFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());

    s_Data.CurrentCommandBuffer->BindPipeline(s_Data.LightingPipeline);

    MeshPushConstants pushConstants = {};
    pushConstants.Data              = glm::vec4(s_Data.MeshCameraDataBuffer.Position, 0.0f);

    s_Data.CurrentCommandBuffer->BindPushConstants(s_Data.LightingPipeline->GetLayout(),
                                                   s_Data.LightingPipeline->GetPushConstantsShaderStageFlags(), 0,
                                                   s_Data.LightingPipeline->GetPushConstantsSize(), &pushConstants);
    s_Data.CurrentCommandBuffer->BindDescriptorSets(s_Data.LightingPipeline->GetLayout(), 0, 1, &s_Data.LightingSet.Handle);
    s_Data.CurrentCommandBuffer->Draw(6);

    s_Data.LightingFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
    s_Data.CurrentCommandBuffer->EndDebugLabel();
}

void VulkanRenderer::ResizeFramebuffersImpl(uint32_t width, uint32_t height)
{
    // Resize all framebuffers

    s_Data.bFramebuffersNeedResize = true;
    s_Data.NewFramebufferSize      = {width, height};
}

const Ref<Image>& VulkanRenderer::GetFinalImageImpl()
{
    return s_Data.LightingFramebuffer->GetAttachments()[0].Attachments[m_Context.GetSwapchain()->GetCurrentFrameIndex()];
}

std::vector<RendererOutput> VulkanRenderer::GetRendererOutputImpl()
{
    std::vector<RendererOutput> rendererOutput;

    const uint32_t currentFrame = m_Context.GetSwapchain()->GetCurrentFrameIndex();

    rendererOutput.emplace_back(s_Data.ShadowMapFramebuffer->GetDepthAttachment(), "ShadowMap");
    rendererOutput.emplace_back(s_Data.GeometryFramebuffer->GetAttachments()[0].Attachments[currentFrame], "Position");
    rendererOutput.emplace_back(s_Data.GeometryFramebuffer->GetAttachments()[1].Attachments[currentFrame], "Normal");
    rendererOutput.emplace_back(s_Data.GeometryFramebuffer->GetAttachments()[2].Attachments[currentFrame], "Albedo");
    rendererOutput.emplace_back(s_Data.GeometryFramebuffer->GetAttachments()[3].Attachments[currentFrame], "Depth");

    return rendererOutput;
}

void VulkanRenderer::Destroy()
{
    m_Context.GetDevice()->WaitDeviceOnFinish();

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        m_Context.GetAllocator()->Unmap(s_Data.UniformCameraDataBuffers[i].Allocation);
        BufferUtils::DestroyBuffer(s_Data.UniformCameraDataBuffers[i]);

        m_Context.GetAllocator()->Unmap(s_Data.UniformPhongModelBuffers[i].Allocation);
        BufferUtils::DestroyBuffer(s_Data.UniformPhongModelBuffers[i]);

        m_Context.GetAllocator()->Unmap(s_Data.UniformShadowsBuffers[i].Allocation);
        BufferUtils::DestroyBuffer(s_Data.UniformShadowsBuffers[i]);
    }

    s_Data.ShadowMapFramebuffer->Destroy();
    s_Data.SetupFramebuffer->Destroy();
    s_Data.GeometryFramebuffer->Destroy();
    //  s_Data.SSAOFramebuffer->Destroy();
    s_Data.LightingFramebuffer->Destroy();

    s_Data.ForwardPassPipeline->Destroy();
    s_Data.ShadowMapPipeline->Destroy();
    s_Data.GeometryPipeline->Destroy();
    //  s_Data.SSAOPipeline->Destroy();
    s_Data.LightingPipeline->Destroy();

    vkDestroyDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), s_Data.ImageDescriptorSetLayout, nullptr);

    s_Data.MeshWhiteTexture->Destroy();
    s_Data.CurrentCommandBuffer = nullptr;

    DestroySkybox();

    for (auto& sampler : s_Data.Samplers)
        vkDestroySampler(m_Context.GetDevice()->GetLogicalDevice(), sampler.second, nullptr);
}

}  // namespace Gauntlet