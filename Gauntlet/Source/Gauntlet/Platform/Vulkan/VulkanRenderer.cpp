#include "GauntletPCH.h"
#include "VulkanRenderer.h"

#include "Gauntlet/Renderer/Camera/PerspectiveCamera.h"

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

#include "Gauntlet/Core/Random.h"

#pragma warning(disable : 4834)

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
    }

    // Geometry pipeline
    {
        FramebufferSpecification geometryFramebufferSpec = {};

        FramebufferAttachmentSpecification attachment = {};
        attachment.LoadOp                             = ELoadOp::LOAD;
        attachment.StoreOp                            = EStoreOp::STORE;
        attachment.ClearColor                         = glm::vec4(0.0f); /*
         attachment.Filter                             = ETextureFilter::NEAREST;
         attachment.Wrap                               = ETextureWrap::CLAMP;*/

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
        s_Data.GeometryDescriptorSetLayout        = GeometryShader->GetDescriptorSetLayouts()[0];
        s_RendererStorage->MeshVertexBufferLayout = GeometryShader->GetVertexBufferLayout();

        PipelineSpecification geometryPipelineSpec = {};
        geometryPipelineSpec.Name                  = "Geometry";
        geometryPipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        geometryPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        geometryPipelineSpec.CullMode              = ECullMode::CULL_MODE_BACK;
        geometryPipelineSpec.bDepthWrite           = VK_TRUE;
        geometryPipelineSpec.bDepthTest            = VK_TRUE;
        geometryPipelineSpec.DepthCompareOp        = ECompareOp::COMPARE_OP_LESS;
        geometryPipelineSpec.TargetFramebuffer     = s_Data.GeometryFramebuffer;
        geometryPipelineSpec.Shader                = GeometryShader;
        geometryPipelineSpec.bDynamicPolygonMode   = VK_TRUE;

        s_Data.GeometryPipeline = MakeRef<VulkanPipeline>(geometryPipelineSpec);
    }

    // Clear-Pass
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
        forwardPipelineSpec.DepthCompareOp        = ECompareOp::COMPARE_OP_LESS;
        forwardPipelineSpec.TargetFramebuffer     = s_Data.GeometryFramebuffer;

        auto MeshShader                = std::static_pointer_cast<VulkanShader>(ShaderLibrary::Load("Resources/Cached/Shaders/Forward"));
        s_Data.MeshDescriptorSetLayout = MeshShader->GetDescriptorSetLayouts()[0];

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

    constexpr VkDeviceSize ShadowsBufferSize = sizeof(ShadowsBuffer);
    s_Data.UniformShadowsBuffers.resize(FRAMES_IN_FLIGHT);
    s_Data.MappedUniformShadowsBuffers.resize(FRAMES_IN_FLIGHT);

    constexpr VkDeviceSize SSAOBufferSize = sizeof(SSAOBuffer);
    s_Data.UniformSSAOBuffers.resize(FRAMES_IN_FLIGHT);
    s_Data.MappedUniformSSAOBuffers.resize(FRAMES_IN_FLIGHT);
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
        BufferUtils::CreateBuffer(EBufferUsageFlags::UNIFORM_BUFFER, ShadowsBufferSize, s_Data.UniformShadowsBuffers[frame],
                                  VMA_MEMORY_USAGE_CPU_ONLY);

        s_Data.MappedUniformShadowsBuffers[frame] = m_Context.GetAllocator()->Map(s_Data.UniformShadowsBuffers[frame].Allocation);

        // SSAO
        BufferUtils::CreateBuffer(EBufferUsageFlags::UNIFORM_BUFFER, SSAOBufferSize, s_Data.UniformSSAOBuffers[frame],
                                  VMA_MEMORY_USAGE_CPU_ONLY);

        s_Data.MappedUniformSSAOBuffers[frame] = m_Context.GetAllocator()->Map(s_Data.UniformSSAOBuffers[frame].Allocation);
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
        shadowMapFramebufferSpec.Width       = 2048;
        shadowMapFramebufferSpec.Height      = 2048;

        s_Data.ShadowMapFramebuffer = MakeRef<VulkanFramebuffer>(shadowMapFramebufferSpec);

        PipelineSpecification shadowmapPipelineSpec = {};
        shadowmapPipelineSpec.Name                  = "ShadowMap";
        shadowmapPipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        shadowmapPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        // shadowmapPipelineSpec.CullMode              = ECullMode::CULL_MODE_FRONT;  // ???
        shadowmapPipelineSpec.bDepthWrite       = VK_TRUE;
        shadowmapPipelineSpec.bDepthTest        = VK_TRUE;
        shadowmapPipelineSpec.DepthCompareOp    = ECompareOp::COMPARE_OP_LESS;
        shadowmapPipelineSpec.TargetFramebuffer = s_Data.ShadowMapFramebuffer;
        shadowmapPipelineSpec.Layout            = Renderer::GetStorageData().MeshVertexBufferLayout;
        shadowmapPipelineSpec.Shader            = ShaderLibrary::Load("Resources/Cached/Shaders/Depth");

        s_Data.ShadowMapPipeline = MakeRef<VulkanPipeline>(shadowmapPipelineSpec);
    }

    // SSAO
    {
        // Generating the kernel(random points in hemisphere: z [0, 1]) we use them in shader for sampling depth
        for (uint32_t i = 0; i < 64; ++i)
        {
            glm::vec3 sample(Random::GetInRange(0.0f, 1.0f) * 2.0 - 1.0, Random::GetInRange(0.0f, 1.0f) * 2.0 - 1.0,
                             Random::GetInRange(0.0f, 1.0f));
            sample = glm::normalize(sample);

            // Scale sample because we want them to be dense to the origin
            auto lerp   = [](float a, float b, float f) -> decltype(auto) { return a + f * (b - a); };
            float scale = (float)i / 64.0f;
            scale       = lerp(0.1f, 1.0f, scale * scale);
            sample *= scale;
            s_Data.SSAODataBuffer.Samples[i] = glm::vec4(sample, 0.0f);
        }

        FramebufferSpecification ssaoFramebufferSpec = {};
        ssaoFramebufferSpec.Name                     = "SSAO";

        FramebufferAttachmentSpecification ssaoFramebufferAttachmentSpec = {};
        ssaoFramebufferAttachmentSpec.Format                             = EImageFormat::R32F;
        ssaoFramebufferAttachmentSpec.Filter                             = ETextureFilter::NEAREST;
        ssaoFramebufferSpec.Attachments                                  = {ssaoFramebufferAttachmentSpec};

        s_Data.SSAOFramebuffer = MakeRef<VulkanFramebuffer>(ssaoFramebufferSpec);

        PipelineSpecification ssaoPipelineSpec = {};
        ssaoPipelineSpec.Name                  = "SSAO";
        ssaoPipelineSpec.Shader                = ShaderLibrary::Load("Resources/Cached/Shaders/SSAO");
        ssaoPipelineSpec.CullMode              = ECullMode::CULL_MODE_BACK;
        ssaoPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        ssaoPipelineSpec.TargetFramebuffer     = s_Data.SSAOFramebuffer;

        s_Data.SSAOPipeline = MakeRef<VulkanPipeline>(ssaoPipelineSpec);
        s_Data.SSAOSet      = &std::static_pointer_cast<VulkanShader>(ssaoPipelineSpec.Shader)->GetDescriptorSets()[0];

        // Next we need to generate a set of random values used to rotate the sample kernel, which will effectively increase the sample
        // count and minimize the 'banding' artifacts mentioned previously.
        std::vector<glm::vec3> ssaoNoise;
        for (uint32_t i = 0; i < 16; ++i)  // 4*4
        {
            glm::vec3 noise(Random::GetInRange(0.0f, 1.0f) * 2.0f - 1.0f, Random::GetInRange(0.0f, 1.0f) * 2.0 - 1.0, 0.0f);
            ssaoNoise.push_back(noise);
        }

        TextureSpecification ssaoNoiseTextureSpec = {};
        ssaoNoiseTextureSpec.CreateTextureID      = true;
        ssaoNoiseTextureSpec.Format               = EImageFormat::RGBA16F;
        ssaoNoiseTextureSpec.Filter               = ETextureFilter::NEAREST;
        ssaoNoiseTextureSpec.Wrap                 = ETextureWrap::REPEAT;
        s_Data.SSAONoiseTexture = MakeRef<VulkanTexture2D>(ssaoNoise.data(), ssaoNoise.size() * sizeof(ssaoNoise[0]), 4, 4);

        for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            s_Data.SSAOPipeline->GetSpecification().Shader->Set("u_PositionMap",
                                                                s_Data.GeometryFramebuffer->GetAttachments()[0].Attachments[frame]);

            s_Data.SSAOPipeline->GetSpecification().Shader->Set("u_NormalMap",
                                                                s_Data.GeometryFramebuffer->GetAttachments()[1].Attachments[frame]);

            s_Data.SSAOPipeline->GetSpecification().Shader->Set("u_TexNoiseMap", s_Data.SSAONoiseTexture);

            s_Data.SSAOPipeline->GetSpecification().Shader->Set("u_UBSSAO", s_Data.UniformSSAOBuffers[frame], sizeof(SSAOBuffer), 0);
        }

        // Blur
        FramebufferSpecification ssaoBlurFramebufferSpec = {};
        ssaoBlurFramebufferSpec.Name                     = "SSAO-Blur";

        FramebufferAttachmentSpecification ssaoBlurFramebufferAttachmentSpec = {};
        ssaoBlurFramebufferAttachmentSpec.Format                             = EImageFormat::R32F;
        ssaoBlurFramebufferAttachmentSpec.Filter                             = ETextureFilter::NEAREST;
        ssaoBlurFramebufferSpec.Attachments                                  = {ssaoBlurFramebufferAttachmentSpec};

        s_Data.SSAOBlurFramebuffer = MakeRef<VulkanFramebuffer>(ssaoBlurFramebufferSpec);

        PipelineSpecification ssaoBlurPipelineSpec = {};
        ssaoBlurPipelineSpec.Name                  = "SSAO-Blur";
        ssaoBlurPipelineSpec.Shader    = std::static_pointer_cast<VulkanShader>(ShaderLibrary::Load("Resources/Cached/Shaders/SSAO-Blur"));
        ssaoBlurPipelineSpec.CullMode  = ECullMode::CULL_MODE_BACK;
        ssaoBlurPipelineSpec.FrontFace = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        ssaoBlurPipelineSpec.TargetFramebuffer = s_Data.SSAOBlurFramebuffer;

        s_Data.SSAOBlurPipeline = MakeRef<VulkanPipeline>(ssaoBlurPipelineSpec);

        s_Data.SSAOBlurSet = &std::static_pointer_cast<VulkanShader>(ssaoBlurPipelineSpec.Shader)->GetDescriptorSets()[0];

        // Updating SSAO-Blur Set
        for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            s_Data.SSAOBlurPipeline->GetSpecification().Shader->Set("u_SSAOMap",
                                                                    s_Data.SSAOFramebuffer->GetAttachments()[0].Attachments[frame]);
        }
    }

    // Lighting
    {
        FramebufferSpecification lightingFramebufferSpec = {};

        FramebufferAttachmentSpecification attachment = {};
        attachment.ClearColor                         = glm::vec4(glm::vec3(0.1f), 1.0f);
        attachment.Format                             = EImageFormat::RGBA16F;
        attachment.LoadOp                             = ELoadOp::CLEAR;
        attachment.StoreOp                            = EStoreOp::STORE;
        lightingFramebufferSpec.Attachments.push_back(attachment);

        lightingFramebufferSpec.Name = "FinalPass";
        s_Data.LightingFramebuffer   = MakeRef<VulkanFramebuffer>(lightingFramebufferSpec);

        PipelineSpecification lightingPipelineSpec = {};
        lightingPipelineSpec.Name                  = "Lighting";
        lightingPipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        lightingPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        lightingPipelineSpec.TargetFramebuffer     = s_Data.LightingFramebuffer;
        lightingPipelineSpec.Shader                = ShaderLibrary::Load("Resources/Cached/Shaders/Lighting");

        s_Data.LightingPipeline = MakeRef<VulkanPipeline>(lightingPipelineSpec);
        s_Data.LightingSet      = &std::static_pointer_cast<VulkanShader>(lightingPipelineSpec.Shader)->GetDescriptorSets()[0];

        for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            s_Data.LightingPipeline->GetSpecification().Shader->Set("u_PositionMap",
                                                                    s_Data.GeometryFramebuffer->GetAttachments()[0].Attachments[frame]);

            s_Data.LightingPipeline->GetSpecification().Shader->Set("u_NormalMap",
                                                                    s_Data.GeometryFramebuffer->GetAttachments()[1].Attachments[frame]);

            s_Data.LightingPipeline->GetSpecification().Shader->Set("u_AlbedoMap",
                                                                    s_Data.GeometryFramebuffer->GetAttachments()[2].Attachments[frame]);

            s_Data.LightingPipeline->GetSpecification().Shader->Set("u_ShadowMap",
                                                                    s_Data.ShadowMapFramebuffer->GetAttachments()[0].Attachments[frame]);
            s_Data.LightingPipeline->GetSpecification().Shader->Set("u_SSAOMap",
                                                                    s_Data.SSAOBlurFramebuffer->GetAttachments()[0].Attachments[frame]);

            s_Data.LightingPipeline->GetSpecification().Shader->Set("u_LightingModelBuffer", s_Data.UniformPhongModelBuffers[frame],
                                                                    sizeof(LightingModelBuffer), 0);
        }
    }
}

void VulkanRenderer::BeginSceneImpl(const Camera& camera)
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
        // ShadowMapFramebuffer has fixed size independent of screen size
        s_Data.GeometryFramebuffer->Resize(s_Data.NewFramebufferSize.x, s_Data.NewFramebufferSize.y);
        s_Data.SetupFramebuffer->Resize(s_Data.NewFramebufferSize.x, s_Data.NewFramebufferSize.y);
        s_Data.SSAOFramebuffer->Resize(s_Data.NewFramebufferSize.x, s_Data.NewFramebufferSize.y);
        s_Data.SSAOBlurFramebuffer->Resize(s_Data.NewFramebufferSize.x, s_Data.NewFramebufferSize.y);

        /*for (auto& geometry : s_Data.SortedGeometry)
        {
            geometry.Material->Invalidate();
        }*/

        // Updating LightingSet
        for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            s_Data.LightingPipeline->GetSpecification().Shader->Set("u_PositionMap",
                                                                    s_Data.GeometryFramebuffer->GetAttachments()[0].Attachments[frame]);

            s_Data.LightingPipeline->GetSpecification().Shader->Set("u_NormalMap",
                                                                    s_Data.GeometryFramebuffer->GetAttachments()[1].Attachments[frame]);

            s_Data.LightingPipeline->GetSpecification().Shader->Set("u_AlbedoMap",
                                                                    s_Data.GeometryFramebuffer->GetAttachments()[2].Attachments[frame]);

            s_Data.LightingPipeline->GetSpecification().Shader->Set("u_ShadowMap",
                                                                    s_Data.ShadowMapFramebuffer->GetAttachments()[0].Attachments[frame]);
            s_Data.LightingPipeline->GetSpecification().Shader->Set(
                "u_SSAOMap", Renderer::GetSettings().BlurSSAO ? s_Data.SSAOBlurFramebuffer->GetAttachments()[0].Attachments[frame]
                                                              : s_Data.SSAOFramebuffer->GetAttachments()[0].Attachments[frame]);

            s_Data.LightingPipeline->GetSpecification().Shader->Set("u_LightingModelBuffer", s_Data.UniformPhongModelBuffers[frame],
                                                                    sizeof(LightingModelBuffer), 0);
        }

        // Updating SSAO Set
        for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            s_Data.SSAOPipeline->GetSpecification().Shader->Set("u_PositionMap",
                                                                s_Data.GeometryFramebuffer->GetAttachments()[0].Attachments[frame]);

            s_Data.SSAOPipeline->GetSpecification().Shader->Set("u_NormalMap",
                                                                s_Data.GeometryFramebuffer->GetAttachments()[1].Attachments[frame]);

            s_Data.SSAOPipeline->GetSpecification().Shader->Set("u_TexNoiseMap", s_Data.SSAONoiseTexture);

            s_Data.SSAOPipeline->GetSpecification().Shader->Set("u_UBSSAO", s_Data.UniformSSAOBuffers[frame], sizeof(SSAOBuffer), 0);
        }

        // Updating SSAO-Blur Set
        for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            s_Data.SSAOBlurPipeline->GetSpecification().Shader->Set("u_SSAOMap",
                                                                    s_Data.SSAOFramebuffer->GetAttachments()[0].Attachments[frame]);
        }

        s_Data.bFramebuffersNeedResize = false;
    }

    // SSAO Enable
    for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
    {
        if (Renderer::GetSettings().EnableSSAO)
            s_Data.LightingPipeline->GetSpecification().Shader->Set(
                "u_SSAOMap", Renderer::GetSettings().BlurSSAO ? s_Data.SSAOBlurFramebuffer->GetAttachments()[0].Attachments[frame]
                                                              : s_Data.SSAOFramebuffer->GetAttachments()[0].Attachments[frame]);
        else
            s_Data.LightingPipeline->GetSpecification().Shader->Set("u_SSAOMap", s_RendererStorage->WhiteTexture);
    }

    // Clear geometry buffer contents
    s_Data.CurrentCommandBuffer->BeginDebugLabel("SetupPass", glm::vec4(0.3f, 0.56f, 0.841f, 1.0f));
    s_Data.SetupFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());

    s_Data.SetupFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
    s_Data.CurrentCommandBuffer->EndDebugLabel();

    {
        s_Data.MeshLightingModelBuffer.Gamma    = s_RendererSettings.Gamma;
        s_Data.MeshLightingModelBuffer.DirLight = DirectionalLight();

        for (uint32_t i = 0; i < s_Data.CurrentPointLightIndex; ++i)
            s_Data.MeshLightingModelBuffer.PointLights[i] = PointLight();
    }
    s_Data.CurrentPointLightIndex = 0;

    s_Data.SortedGeometry.clear();
}

void VulkanRenderer::SubmitMeshImpl(const Ref<Mesh>& mesh, const glm::mat4& transform)
{
    for (uint32_t i = 0; i < mesh->GetSubmeshCount(); ++i)
    {
        s_Data.SortedGeometry.emplace_back(mesh->GetMaterial(i), mesh->GetVertexBuffers()[i], mesh->GetIndexBuffers()[i], transform);
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

    const bool bIsActive = color.x > 0.0f || color.y > 0.0f || color.z || 0.0f;
    s_Data.MeshLightingModelBuffer.PointLights[s_Data.CurrentPointLightIndex].CLQActive = glm::vec4(CLQ, bIsActive);

    ++s_Data.CurrentPointLightIndex;
}

void VulkanRenderer::AddDirectionalLightImpl(const glm::vec3& color, const glm::vec3& direction,
                                             const glm::vec4& AmbientSpecularShininessShadows)
{
    s_Data.MeshLightingModelBuffer.Gamma = s_RendererSettings.Gamma;

    s_Data.MeshLightingModelBuffer.DirLight.Color                    = glm::vec4(color, 0.0f);
    s_Data.MeshLightingModelBuffer.DirLight.Direction                = glm::vec4(direction, 0.0f);
    s_Data.MeshLightingModelBuffer.DirLight.AmbientSpecularShininess = AmbientSpecularShininessShadows;
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
    PipelineSpec.DepthCompareOp = ECompareOp::COMPARE_OP_LESS;

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

    s_Data.CurrentCommandBuffer->BindDescriptorSets(s_Data.SkyboxPipeline->GetLayout(), 0, 1, &s_Data.SkyboxDescriptorSet.Handle);

    const auto& CubeMesh = s_Data.DefaultSkybox->GetCubeMesh();
    VkDeviceSize Offset  = 0;

    VkBuffer vb = (VkBuffer)CubeMesh->GetVertexBuffers()[0]->Get();
    s_Data.CurrentCommandBuffer->BindVertexBuffers(0, 1, &vb, &Offset);

    VkBuffer ib = (VkBuffer)CubeMesh->GetIndexBuffers()[0]->Get();
    s_Data.CurrentCommandBuffer->BindIndexBuffer(ib);
    s_Data.CurrentCommandBuffer->DrawIndexed(static_cast<uint32_t>(CubeMesh->GetIndexBuffers()[0]->GetCount()));

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

    // ShadowMap-Pass
    {
        s_Data.ShadowMapFramebuffer->SetDepthStencilClearColor(GetSettings().RenderShadows ? 1.0f : 0.0f, 0);

        s_Data.CurrentCommandBuffer->BeginDebugLabel("ShadowMapPass", glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
        if (!Renderer::GetSettings().RenderShadows)
        {
            s_Data.ShadowMapFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());
        }
        else
        {
            s_Data.ShadowMapFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());

            constexpr float cameraWidth     = 16.0f;
            constexpr float zNear           = 0.00001f;
            constexpr float zFar            = 512.0f;
            const glm::mat4 lightProjection = glm::ortho(-cameraWidth, cameraWidth, -cameraWidth, cameraWidth, zNear, zFar);

            const glm::mat4 lightView =
                glm::lookAt(glm::vec3(s_Data.MeshLightingModelBuffer.DirLight.Direction), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            s_Data.MeshShadowsBuffer.LightSpaceMatrix = lightProjection * lightView;

            LightPushConstants pushConstants   = {};
            pushConstants.LightSpaceProjection = s_Data.MeshShadowsBuffer.LightSpaceMatrix;
            pushConstants.Model                = glm::mat4(1.0f);

            for (auto& geometry : s_Data.SortedGeometry)
            {
                s_Data.CurrentCommandBuffer->SetPipelinePolygonMode(s_Data.ShadowMapPipeline, GetSettings().ShowWireframes
                                                                                                  ? EPolygonMode::POLYGON_MODE_LINE
                                                                                                  : EPolygonMode::POLYGON_MODE_FILL);
                s_Data.CurrentCommandBuffer->BindPipeline(s_Data.ShadowMapPipeline);

                s_Data.CurrentCommandBuffer->BindPushConstants(s_Data.ShadowMapPipeline->GetLayout(),
                                                               s_Data.ShadowMapPipeline->GetPushConstantsShaderStageFlags(), 0,
                                                               s_Data.ShadowMapPipeline->GetPushConstantsSize(), &pushConstants);

                VkDeviceSize Offset = 0;
                VkBuffer vb         = (VkBuffer)geometry.VertexBuffer->Get();
                s_Data.CurrentCommandBuffer->BindVertexBuffers(0, 1, &vb, &Offset);

                VkBuffer ib = (VkBuffer)geometry.IndexBuffer->Get();
                s_Data.CurrentCommandBuffer->BindIndexBuffer(ib);
                s_Data.CurrentCommandBuffer->DrawIndexed(static_cast<uint32_t>(geometry.IndexBuffer->GetCount()));
                ++Renderer::GetStats().DrawCalls;
            }
        }

        s_Data.ShadowMapFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
        s_Data.CurrentCommandBuffer->EndDebugLabel();
    }

    // Actual Geometry-Pass
    {
        s_Data.CurrentCommandBuffer->BeginDebugLabel("GeometryPass", glm::vec4(0.5f, 0.0f, 0.0f, 1.0f));
        s_Data.GeometryFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());

        // Set shadow buffer(only contains light space mvp mat4)
        // This currently doesn't work in Geometry shader only in Forward
        // memcpy(s_Data.MappedUniformShadowsBuffers[m_Context.GetSwapchain()->GetCurrentFrameIndex()], &s_Data.MeshShadowsBuffer,
        //        sizeof(ShadowsBuffer));

        // Set lighting stuff
        memcpy(s_Data.MappedUniformPhongModelBuffers[m_Context.GetSwapchain()->GetCurrentFrameIndex()], &s_Data.MeshLightingModelBuffer,
               sizeof(LightingModelBuffer));

        for (auto& geometry : s_Data.SortedGeometry)
        {

            s_Data.CurrentCommandBuffer->SetPipelinePolygonMode(
                s_Data.GeometryPipeline, GetSettings().ShowWireframes ? EPolygonMode::POLYGON_MODE_LINE : EPolygonMode::POLYGON_MODE_FILL);
            s_Data.CurrentCommandBuffer->BindPipeline(s_Data.GeometryPipeline);

            MeshPushConstants PushConstants = {};
            PushConstants.TransformMatrix   = geometry.Transform;

            s_Data.CurrentCommandBuffer->BindPushConstants(s_Data.GeometryPipeline->GetLayout(),
                                                           s_Data.GeometryPipeline->GetPushConstantsShaderStageFlags(), 0,
                                                           s_Data.GeometryPipeline->GetPushConstantsSize(), &PushConstants);

            VkDescriptorSet ds = (VkDescriptorSet)geometry.Material->GetDescriptorSet();
            s_Data.CurrentCommandBuffer->BindDescriptorSets(s_Data.GeometryPipeline->GetLayout(), 0, 1, &ds);

            VkDeviceSize Offset = 0;

            VkBuffer vb = (VkBuffer)geometry.VertexBuffer->Get();
            s_Data.CurrentCommandBuffer->BindVertexBuffers(0, 1, &vb, &Offset);

            VkBuffer ib = (VkBuffer)geometry.IndexBuffer->Get();
            s_Data.CurrentCommandBuffer->BindIndexBuffer(ib);
            s_Data.CurrentCommandBuffer->DrawIndexed(static_cast<uint32_t>(geometry.IndexBuffer->GetCount()));
            ++Renderer::GetStats().DrawCalls;
        }

        s_Data.GeometryFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
        s_Data.CurrentCommandBuffer->EndDebugLabel();
    }
}

void VulkanRenderer::FlushImpl()
{
    // Render skybox as last geometry to prevent depth testing as mush as possible
    s_Data.CurrentCommandBuffer->BeginDebugLabel("Skybox", glm::vec4(0.5f, 0.0f, 0.0f, 1.0f));
    s_Data.GeometryFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());

    DrawSkybox();

    s_Data.GeometryFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
    s_Data.CurrentCommandBuffer->EndDebugLabel();

    // TODO: Make assertions on DescriptorSets usage

    // SSAO-Pass
    if (!s_Data.SortedGeometry.empty() && Renderer::GetSettings().EnableSSAO)
    {
        static constexpr auto noiseFactor             = 4.0f;
        s_Data.SSAODataBuffer.ViewportSizeNoiseFactor = glm::vec4(s_Data.NewFramebufferSize, noiseFactor, 0.0f);
        s_Data.SSAODataBuffer.CameraProjection        = s_Data.MeshCameraDataBuffer.Projection;

        memcpy(s_Data.MappedUniformSSAOBuffers[m_Context.GetSwapchain()->GetCurrentFrameIndex()], &s_Data.SSAODataBuffer,
               sizeof(SSAOBuffer));

        s_Data.CurrentCommandBuffer->BeginDebugLabel("SSAO", glm::vec4(glm::vec3(0.5f), 1.0f));
        s_Data.SSAOFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());

        s_Data.CurrentCommandBuffer->BindPipeline(s_Data.SSAOPipeline);

        s_Data.CurrentCommandBuffer->BindDescriptorSets(s_Data.SSAOPipeline->GetLayout(), 0, 1, &s_Data.SSAOSet->Handle);
        s_Data.CurrentCommandBuffer->Draw(3);

        s_Data.SSAOFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
        s_Data.CurrentCommandBuffer->EndDebugLabel();

        if (Renderer::GetSettings().BlurSSAO)
        {
            s_Data.CurrentCommandBuffer->BeginDebugLabel("SSAO-Blur", glm::vec4(glm::vec3(0.8f), 1.0f));
            s_Data.SSAOBlurFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());

            s_Data.CurrentCommandBuffer->BindPipeline(s_Data.SSAOBlurPipeline);

            s_Data.CurrentCommandBuffer->BindDescriptorSets(s_Data.SSAOBlurPipeline->GetLayout(), 0, 1, &s_Data.SSAOBlurSet->Handle);
            s_Data.CurrentCommandBuffer->Draw(3);

            s_Data.SSAOBlurFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
            s_Data.CurrentCommandBuffer->EndDebugLabel();
        }
    }

    // Final Lighting-Pass
    {
        s_Data.CurrentCommandBuffer->BeginDebugLabel("LightingPass", glm::vec4(0.7f, 0.7f, 0.0f, 1.0f));
        s_Data.LightingFramebuffer->BeginRenderPass(s_Data.CurrentCommandBuffer->Get());

        s_Data.CurrentCommandBuffer->BindPipeline(s_Data.LightingPipeline);

        MeshPushConstants pushConstants = {};
        pushConstants.Data              = glm::vec4(s_Data.MeshCameraDataBuffer.Position, 0.0f);
        pushConstants.TransformMatrix   = s_Data.MeshShadowsBuffer.LightSpaceMatrix;

        s_Data.CurrentCommandBuffer->BindPushConstants(s_Data.LightingPipeline->GetLayout(),
                                                       s_Data.LightingPipeline->GetPushConstantsShaderStageFlags(), 0,
                                                       s_Data.LightingPipeline->GetPushConstantsSize(), &pushConstants);
        s_Data.CurrentCommandBuffer->BindDescriptorSets(s_Data.LightingPipeline->GetLayout(), 0, 1, &s_Data.LightingSet->Handle);
        s_Data.CurrentCommandBuffer->Draw(3);

        s_Data.LightingFramebuffer->EndRenderPass(s_Data.CurrentCommandBuffer->Get());
        s_Data.CurrentCommandBuffer->EndDebugLabel();
    }
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
    rendererOutput.emplace_back(s_Data.SSAOFramebuffer->GetAttachments()[0].Attachments[currentFrame], "SSAO");
    rendererOutput.emplace_back(s_Data.SSAOBlurFramebuffer->GetAttachments()[0].Attachments[currentFrame], "SSAO-Blur");

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

        m_Context.GetAllocator()->Unmap(s_Data.UniformSSAOBuffers[i].Allocation);
        BufferUtils::DestroyBuffer(s_Data.UniformSSAOBuffers[i]);
    }

    s_Data.ShadowMapFramebuffer->Destroy();
    s_Data.SetupFramebuffer->Destroy();
    s_Data.GeometryFramebuffer->Destroy();
    s_Data.LightingFramebuffer->Destroy();

    s_Data.ForwardPassPipeline->Destroy();
    s_Data.ShadowMapPipeline->Destroy();
    s_Data.GeometryPipeline->Destroy();
    s_Data.LightingPipeline->Destroy();

    vkDestroyDescriptorSetLayout(m_Context.GetDevice()->GetLogicalDevice(), s_Data.ImageDescriptorSetLayout, nullptr);

    s_Data.SSAOFramebuffer->Destroy();
    s_Data.SSAOPipeline->Destroy();
    s_Data.SSAONoiseTexture->Destroy();

    s_Data.SSAOBlurPipeline->Destroy();
    s_Data.SSAOBlurFramebuffer->Destroy();

    s_Data.CurrentCommandBuffer = nullptr;

    DestroySkybox();

    for (auto& sampler : s_Data.Samplers)
        vkDestroySampler(m_Context.GetDevice()->GetLogicalDevice(), sampler.second, nullptr);
}

}  // namespace Gauntlet