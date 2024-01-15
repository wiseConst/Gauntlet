#include "GauntletPCH.h"
#include "Renderer.h"
#include "Shader.h"
#include "Texture.h"
#include "Pipeline.h"
#include "Framebuffer.h"
#include "Mesh.h"
#include "Camera/Camera.h"

#include "Material.h"
#include "Renderer2D.h"
#include "GraphicsContext.h"
#include "CommandBuffer.h"

#include "ParticleSystem.h"

#include "Gauntlet/Core/Random.h"
#include "Animation.h"

#include "Gauntlet/Platform/Vulkan/VulkanRenderer.h"
#include "Gauntlet/Platform/Vulkan/VulkanCommandBuffer.h"

namespace Gauntlet
{

// Static storage
Renderer* Renderer::s_Renderer = nullptr;
std::mutex Renderer::s_ResourceAccessMutex;

Renderer::RendererStats Renderer::s_RendererStats;
Renderer::RendererSettings Renderer::s_RendererSettings;
Renderer::RendererStorage* Renderer::s_RendererStorage = new Renderer::RendererStorage();

void Renderer::Init()
{
    ShaderLibrary::Init();
    SamplerStorage::Initialize();

    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            s_Renderer = new VulkanRenderer();
            break;
        }
        case RendererAPI::EAPI::None:
        {
            LOG_ERROR("RendererAPI::EAPI::None!");
            GNT_ASSERT(false, "Unknown RendererAPI!");
            break;
        }
    }

    s_RendererStorage->UploadHeap = StagingBuffer::Create(1024 * 1024);  // 1 MB

    for (auto& cameraUB : s_RendererStorage->CameraUniformBuffer)
    {
        cameraUB = UniformBuffer::Create(sizeof(UBCamera));
        cameraUB->Map(true);
    }

    {
        const uint32_t WhiteTexutreData = 0xffffffff;
        s_RendererStorage->WhiteTexture = Texture2D::Create(&WhiteTexutreData, sizeof(WhiteTexutreData), 1, 1);
    }

    // GBuffer
    {
        FramebufferSpecification geometryFramebufferSpec = {};

        FramebufferAttachmentSpecification attachment = {};
        geometryFramebufferSpec.Name                  = "GBuffer";
        attachment.LoadOp                             = ELoadOp::LOAD;
        attachment.StoreOp                            = EStoreOp::STORE;
        attachment.ClearColor                         = glm::vec4(1.0f);
        attachment.Filter                             = ETextureFilter::NEAREST;

        // Position
        attachment.Format = EImageFormat::RGBA16F;
        geometryFramebufferSpec.Attachments.push_back(attachment);

        // Normal
        attachment.Format = EImageFormat::RGBA16F;
        geometryFramebufferSpec.Attachments.push_back(attachment);

        // Albedo
        attachment.Format = EImageFormat::RGBA;
        geometryFramebufferSpec.Attachments.push_back(attachment);

        // Metallic Roughness AO
        attachment.Format = EImageFormat::RGBA;
        geometryFramebufferSpec.Attachments.push_back(attachment);

        // Depth
        attachment.Format = EImageFormat::DEPTH32F;
        geometryFramebufferSpec.Attachments.push_back(attachment);

        for (auto& fb : s_RendererStorage->GeometryFramebuffer)
            fb = Framebuffer::Create(geometryFramebufferSpec);

        auto GeometryShader                             = ShaderLibrary::Load("Geometry");
        s_RendererStorage->StaticMeshVertexBufferLayout = GeometryShader->GetVertexBufferLayout();

        PipelineSpecification geometryPipelineSpec = {};
        geometryPipelineSpec.Name                  = "GeometryDeferred";
        geometryPipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        geometryPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        geometryPipelineSpec.CullMode              = ECullMode::CULL_MODE_BACK;
        geometryPipelineSpec.bDepthWrite           = true;
        geometryPipelineSpec.bDepthTest            = true;
        geometryPipelineSpec.DepthCompareOp        = ECompareOp::COMPARE_OP_LESS;
        geometryPipelineSpec.TargetFramebuffer     = s_RendererStorage->GeometryFramebuffer;
        geometryPipelineSpec.Shader                = GeometryShader;
        geometryPipelineSpec.bDynamicPolygonMode   = true;

        s_RendererStorage->GeometryPipeline = Pipeline::Create(geometryPipelineSpec);
    }

    // Directional ShadowMap
    {
        FramebufferSpecification shadowMapFramebufferSpec = {};

        FramebufferAttachmentSpecification shadowMapAttachment = {};
        shadowMapAttachment.ClearColor                         = glm::vec4(1.0f);
        shadowMapAttachment.Format                             = EImageFormat::DEPTH32F;
        shadowMapAttachment.LoadOp                             = ELoadOp::CLEAR;
        shadowMapAttachment.StoreOp                            = EStoreOp::STORE;
        shadowMapAttachment.Filter                             = ETextureFilter::LINEAR;
        shadowMapAttachment.Wrap                               = ETextureWrap::CLAMP_TO_EDGE;

        shadowMapFramebufferSpec.Attachments = {shadowMapAttachment};
        shadowMapFramebufferSpec.Name        = "ShadowMap";
        shadowMapFramebufferSpec.Width       = shadowMapFramebufferSpec.Height =
            s_RendererSettings.Shadows.ShadowPresets[s_RendererSettings.Shadows.CurrentShadowPreset].second;

        for (auto& fb : s_RendererStorage->ShadowMapFramebuffer)
            fb = Framebuffer::Create(shadowMapFramebufferSpec);

        PipelineSpecification shadowmapPipelineSpec = {};
        shadowmapPipelineSpec.Name                  = "ShadowMap";
        shadowmapPipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        shadowmapPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        shadowmapPipelineSpec.CullMode              = ECullMode::CULL_MODE_FRONT;
        shadowmapPipelineSpec.bDepthWrite           = true;
        shadowmapPipelineSpec.bDepthTest            = true;
        shadowmapPipelineSpec.DepthCompareOp        = ECompareOp::COMPARE_OP_LESS;
        shadowmapPipelineSpec.TargetFramebuffer     = s_RendererStorage->ShadowMapFramebuffer;
        shadowmapPipelineSpec.Layout                = s_RendererStorage->StaticMeshVertexBufferLayout;
        shadowmapPipelineSpec.Shader                = ShaderLibrary::Load("DirShadowMap");

        s_RendererStorage->ShadowMapPipeline = Pipeline::Create(shadowmapPipelineSpec);

        for (auto& shadowsUB : s_RendererStorage->ShadowsUniformBuffer)
        {
            shadowsUB = UniformBuffer::Create(sizeof(UBShadows));
            shadowsUB->Map();
        }
    }

    // SSAO
    {
        FramebufferSpecification ssaoFramebufferSpec = {};
        ssaoFramebufferSpec.Name                     = "SSAO";

        FramebufferAttachmentSpecification ssaoFramebufferAttachmentSpec = {};
        ssaoFramebufferAttachmentSpec.Format                             = EImageFormat::R8;
        ssaoFramebufferAttachmentSpec.Filter                             = ETextureFilter::NEAREST;
        ssaoFramebufferSpec.Attachments                                  = {ssaoFramebufferAttachmentSpec};

        for (auto& fb : s_RendererStorage->SSAOFramebuffer)
            fb = Framebuffer::Create(ssaoFramebufferSpec);

        PipelineSpecification ssaoPipelineSpec = {};
        ssaoPipelineSpec.Name                  = "SSAO";
        ssaoPipelineSpec.Shader                = ShaderLibrary::Load("SSAO");
        ssaoPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        ssaoPipelineSpec.TargetFramebuffer     = s_RendererStorage->SSAOFramebuffer;

        s_RendererStorage->SSAOPipeline = Pipeline::Create(ssaoPipelineSpec);

        for (auto& ssaoUB : s_RendererStorage->SSAOUniformBuffer)
        {
            ssaoUB = UniformBuffer::Create(sizeof(UBSSAO));
            ssaoUB->Map(true);
        }
    }

    // SSAO-Blur
    {
        FramebufferSpecification ssaoBlurFramebufferSpec = {};
        ssaoBlurFramebufferSpec.Name                     = "SSAO-Blur";

        FramebufferAttachmentSpecification ssaoBlurFramebufferAttachmentSpec = {};
        ssaoBlurFramebufferAttachmentSpec.Format                             = EImageFormat::R8;
        ssaoBlurFramebufferAttachmentSpec.Filter                             = ETextureFilter::NEAREST;
        ssaoBlurFramebufferSpec.Attachments                                  = {ssaoBlurFramebufferAttachmentSpec};

        for (auto& fb : s_RendererStorage->SSAOBlurFramebuffer)
            fb = Framebuffer::Create(ssaoBlurFramebufferSpec);

        PipelineSpecification ssaoBlurPipelineSpec = {};
        ssaoBlurPipelineSpec.Name                  = "SSAO-Blur";
        ssaoBlurPipelineSpec.Shader                = ShaderLibrary::Load("SSAO-Blur");
        ssaoBlurPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        ssaoBlurPipelineSpec.TargetFramebuffer     = s_RendererStorage->SSAOBlurFramebuffer;

        s_RendererStorage->SSAOBlurPipeline = Pipeline::Create(ssaoBlurPipelineSpec);

        // Next we need to generate a set of random values used to rotate the sample kernel, which will effectively increase the sample
        // count and minimize the 'banding' artifacts mentioned previously.
        std::vector<glm::vec4> ssaoNoise(s_SSAO_KERNEL_SIZE);
        for (uint32_t i = 0; i < s_SSAO_KERNEL_SIZE; ++i)  // 4*4 blur
        {
            ssaoNoise[i] = glm::vec4(Random::GetInRange0To1() * 2.0f - 1.0f, Random::GetInRange0To1() * 2.0f - 1.0f, 0.0f, 1.0f);
        }

        TextureSpecification ssaoNoiseTextureSpec = {};
        ssaoNoiseTextureSpec.CreateTextureID      = true;
        ssaoNoiseTextureSpec.Format               = EImageFormat::RGBA32F;
        ssaoNoiseTextureSpec.Filter               = ETextureFilter::NEAREST;
        ssaoNoiseTextureSpec.Wrap                 = ETextureWrap::REPEAT;
        s_RendererStorage->SSAONoiseTexture =
            Texture2D::Create(ssaoNoise.data(), ssaoNoise.size() * sizeof(ssaoNoise[0]), 4, 4, ssaoNoiseTextureSpec);
    }

    // Lighting
    {
        FramebufferSpecification lightingFramebufferSpec = {};

        FramebufferAttachmentSpecification attachment = {};
        attachment.ClearColor                         = glm::vec4(glm::vec3(0.1f), 1.0f);
        attachment.Format                             = EImageFormat::R11G11B10;  // EImageFormat::RGBA16F;
        attachment.LoadOp                             = ELoadOp::CLEAR;
        attachment.StoreOp                            = EStoreOp::STORE;
        lightingFramebufferSpec.Attachments.push_back(attachment);

        lightingFramebufferSpec.Name = "LightingDeferred";
        for (auto& fb : s_RendererStorage->LightingFramebuffer)
            fb = Framebuffer::Create(lightingFramebufferSpec);

        PipelineSpecification lightingPipelineSpec = {};
        lightingPipelineSpec.Name                  = "Lighting";
        lightingPipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        lightingPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        lightingPipelineSpec.TargetFramebuffer     = s_RendererStorage->LightingFramebuffer;
        lightingPipelineSpec.Shader                = ShaderLibrary::Load("Lighting");

        s_RendererStorage->LightingPipeline = Pipeline::Create(lightingPipelineSpec);

        for (auto& lightingUB : s_RendererStorage->LightingUniformBuffer)
        {
            lightingUB = UniformBuffer::Create(sizeof(UBLighting));
            lightingUB->Map(true);
        }
    }

    // Chromatic Aberration
    {
        FramebufferSpecification caFramebufferSpec    = {};
        FramebufferAttachmentSpecification attachment = {};
        attachment.ClearColor                         = glm::vec4(0.0f);
        attachment.Format                             = EImageFormat::R11G11B10;
        attachment.LoadOp                             = ELoadOp::CLEAR;
        attachment.StoreOp                            = EStoreOp::STORE;
        caFramebufferSpec.Attachments.push_back(attachment);
        caFramebufferSpec.Name = "ChromaticAbberation";

        for (auto& fb : s_RendererStorage->ChromaticAberrationFramebuffer)
            fb = Framebuffer::Create(caFramebufferSpec);

        PipelineSpecification caPipelineSpec = {};
        caPipelineSpec.Name                  = "ChromaticAbberation";
        caPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        caPipelineSpec.TargetFramebuffer     = s_RendererStorage->ChromaticAberrationFramebuffer;
        caPipelineSpec.Shader                = ShaderLibrary::Load("ChromaticAberration");

        s_RendererStorage->ChromaticAberrationPipeline = Pipeline::Create(caPipelineSpec);
    }

    // Clear-Pass
    {
        FramebufferSpecification setupFramebufferSpec = {};
        setupFramebufferSpec.Name                     = "Setup";
        setupFramebufferSpec.LoadOp                   = ELoadOp::CLEAR;
        setupFramebufferSpec.StoreOp                  = EStoreOp::STORE;

        for (uint32_t i = 0; i < s_RendererStorage->SetupFramebuffer.size(); ++i)
        {
            setupFramebufferSpec.ExistingAttachments = s_RendererStorage->GeometryFramebuffer[i]->GetAttachments();
            s_RendererStorage->SetupFramebuffer[i]   = Framebuffer::Create(setupFramebufferSpec);
        }
    }

    // PBR
    {
        FramebufferSpecification pbrForwardFramebufferSpec = {};

        FramebufferAttachmentSpecification attachment = {};
        attachment.LoadOp                             = ELoadOp::CLEAR;
        attachment.StoreOp                            = EStoreOp::STORE;
        attachment.ClearColor                         = glm::vec4(0.0f);

        attachment.Format = EImageFormat::RGBA;
        pbrForwardFramebufferSpec.Attachments.push_back(attachment);

        // Depth
        attachment.ClearColor = glm::vec4(1.0f, glm::vec3(0.0f));
        attachment.Format     = EImageFormat::DEPTH32F;
        pbrForwardFramebufferSpec.Attachments.push_back(attachment);

        pbrForwardFramebufferSpec.Name = "PBR-Forward";
        for (auto& fb : s_RendererStorage->PBRFramebuffer)
            fb = Framebuffer::Create(pbrForwardFramebufferSpec);

        auto PBRShader = ShaderLibrary::Load("PBR");

        PipelineSpecification pbrPipelineSpec = {};
        pbrPipelineSpec.Name                  = "GBuffer";
        pbrPipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        pbrPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        pbrPipelineSpec.CullMode              = ECullMode::CULL_MODE_BACK;
        pbrPipelineSpec.bDepthWrite           = true;
        pbrPipelineSpec.bDepthTest            = true;
        pbrPipelineSpec.DepthCompareOp        = ECompareOp::COMPARE_OP_LESS;
        pbrPipelineSpec.TargetFramebuffer     = s_RendererStorage->PBRFramebuffer;
        pbrPipelineSpec.Shader                = PBRShader;
        pbrPipelineSpec.bDynamicPolygonMode   = true;

        s_RendererStorage->PBRPipeline = Pipeline::Create(pbrPipelineSpec);
    }

    for (auto& commandBuffer : s_RendererStorage->RenderCommandBuffer)
        commandBuffer = CommandBuffer::Create(ECommandBufferType::COMMAND_BUFFER_TYPE_GRAPHICS);

    s_RendererStats.PipelineStatisticsResults.resize(s_RendererStorage->RenderCommandBuffer[0]->GetPipelineStatisticsResults().size());

    s_RendererStorage->GPUParticleSystem = MakeRef<ParticleSystem>();

    // Animation
    /*  {
          PipelineSpecification animationPipelineSpec = {};
          animationPipelineSpec.Name                  = "Animation";
          animationPipelineSpec.TargetFramebuffer     = s_RendererStorage->GeometryFramebuffer;
          animationPipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
          animationPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
          animationPipelineSpec.CullMode              = ECullMode::CULL_MODE_BACK;
          animationPipelineSpec.bDepthWrite           = true;
          animationPipelineSpec.bDepthTest            = true;
          animationPipelineSpec.DepthCompareOp        = ECompareOp::COMPARE_OP_LESS;
          animationPipelineSpec.bDynamicPolygonMode   = true;

          auto animationShader                          = ShaderLibrary::Load("Resources/Cached/Shaders/Animated");
          s_RendererStorage->AnimatedVertexBufferLayout = animationShader->GetVertexBufferLayout();

          animationPipelineSpec.Shader = animationShader;

          s_RendererStorage->AnimationPipeline = Pipeline::Create(animationPipelineSpec);
      }*/
}

void Renderer::Shutdown()
{
    GraphicsContext::Get().WaitDeviceOnFinish();

    ShaderLibrary::Shutdown();

    for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
    {
        s_RendererStorage->GeometryFramebuffer[frame]->Destroy();

        s_RendererStorage->SetupFramebuffer[frame]->Destroy();
        s_RendererStorage->PBRFramebuffer[frame]->Destroy();
        s_RendererStorage->ShadowMapFramebuffer[frame]->Destroy();
        s_RendererStorage->SSAOFramebuffer[frame]->Destroy();
        s_RendererStorage->SSAOBlurFramebuffer[frame]->Destroy();
        s_RendererStorage->LightingFramebuffer[frame]->Destroy();
        s_RendererStorage->ChromaticAberrationFramebuffer[frame]->Destroy();
    }

    s_RendererStorage->GPUParticleSystem->Destroy();

    s_RendererStorage->UploadHeap->Destroy();

    //  s_RendererStorage->AnimationPipeline->Destroy();

    s_RendererStorage->GeometryPipeline->Destroy();
    s_RendererStorage->PBRPipeline->Destroy();

    s_RendererStorage->ShadowMapPipeline->Destroy();
    for (auto& ub : s_RendererStorage->ShadowsUniformBuffer)
        ub->Destroy();

    s_RendererStorage->SSAOPipeline->Destroy();
    for (auto& ub : s_RendererStorage->SSAOUniformBuffer)
        ub->Destroy();

    s_RendererStorage->SSAONoiseTexture->Destroy();
    s_RendererStorage->SSAOBlurPipeline->Destroy();

    s_RendererStorage->LightingPipeline->Destroy();
    for (auto& ub : s_RendererStorage->LightingUniformBuffer)
        ub->Destroy();

    s_RendererStorage->ChromaticAberrationPipeline->Destroy();

    for (auto& ub : s_RendererStorage->CameraUniformBuffer)
        ub->Destroy();

    s_RendererStorage->WhiteTexture->Destroy();
    SamplerStorage::Destroy();

    delete s_RendererStorage;
    s_RendererStorage = nullptr;

    delete s_Renderer;
    s_Renderer = nullptr;
}

const Ref<Image>& Renderer::GetFinalImage()
{
    if (s_RendererSettings.ChromaticAberrationView)
        return s_RendererStorage->ChromaticAberrationFramebuffer[s_RendererStorage->CurrentFrame]->GetAttachments()[0].Attachment;
    return s_RendererStorage->LightingFramebuffer[s_RendererStorage->CurrentFrame]->GetAttachments()[0].Attachment;
}

void Renderer::Begin()
{
    s_Renderer->BeginImpl();

    s_RendererStorage->CurrentFrame = GraphicsContext::Get().GetCurrentFrameIndex();
    s_RendererStats.DrawCalls       = 0;
    s_RendererStats.PassStatistsics.clear();
    if (s_RendererStorage->UploadHeap->GetCapacity() > s_RendererStats.s_MaxUploadHeapSizeMB)
        s_RendererStorage->UploadHeap->Resize(s_RendererStats.s_MaxUploadHeapSizeMB);

    s_RendererStats.UploadHeapCapacity = s_RendererStorage->UploadHeap->GetCapacity();
    for (auto& currentStat : s_RendererStats.PipelineStatisticsResults)
        currentStat = 0;

    if (s_RendererSettings.Shadows.ShadowPresets[s_RendererSettings.Shadows.CurrentShadowPreset].second !=
        s_RendererStorage->ShadowMapFramebuffer[s_RendererStorage->CurrentFrame]->GetWidth())
    {
        s_RendererStorage->ShadowMapFramebuffer[s_RendererStorage->CurrentFrame]->Resize(
            s_RendererSettings.Shadows.ShadowPresets[s_RendererSettings.Shadows.CurrentShadowPreset].second,
            s_RendererSettings.Shadows.ShadowPresets[s_RendererSettings.Shadows.CurrentShadowPreset].second);
    }

    if (s_RendererStorage->bFramebuffersNeedResize)
    {
        for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            s_RendererStorage->GeometryFramebuffer[frame]->Resize(s_RendererStorage->NewFramebufferSize.x,
                                                                  s_RendererStorage->NewFramebufferSize.y);
            s_RendererStorage->SetupFramebuffer[frame]->Resize(s_RendererStorage->NewFramebufferSize.x,
                                                               s_RendererStorage->NewFramebufferSize.y);
            s_RendererStorage->SSAOFramebuffer[frame]->Resize(s_RendererStorage->NewFramebufferSize.x,
                                                              s_RendererStorage->NewFramebufferSize.y);
            s_RendererStorage->SSAOBlurFramebuffer[frame]->Resize(s_RendererStorage->NewFramebufferSize.x,
                                                                  s_RendererStorage->NewFramebufferSize.y);
            s_RendererStorage->LightingFramebuffer[frame]->Resize(s_RendererStorage->NewFramebufferSize.x,
                                                                  s_RendererStorage->NewFramebufferSize.y);
            s_RendererStorage->ChromaticAberrationFramebuffer[frame]->Resize(s_RendererStorage->NewFramebufferSize.x,
                                                                             s_RendererStorage->NewFramebufferSize.y);

            s_RendererStorage->PBRFramebuffer[frame]->Resize(s_RendererStorage->NewFramebufferSize.x,
                                                             s_RendererStorage->NewFramebufferSize.y);
        }

        /*for (auto& geometry : s_Data.SortedGeometry)
        {
            geometry.Material->Invalidate();
        }*/

        s_RendererStorage->bFramebuffersNeedResize = false;
    }

    // SSAO Enable
    s_RendererStorage->SSAOPipeline->GetSpecification().Shader->Set("u_TexNoiseMap", s_RendererStorage->SSAONoiseTexture);
    if (Renderer::GetSettings().AO.EnableSSAO && !s_RendererStorage->SortedGeometry.empty())
    {
        if (Renderer::GetSettings().AO.BlurSSAO)
        {
            s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set(
                "u_SSAOMap", s_RendererStorage->SSAOBlurFramebuffer[s_RendererStorage->CurrentFrame]->GetAttachments()[0].Attachment);
        }
        else
        {
            s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set(
                "u_SSAOMap", s_RendererStorage->SSAOFramebuffer[s_RendererStorage->CurrentFrame]->GetAttachments()[0].Attachment);
        }
    }
    else
    {
        s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set("u_SSAOMap", s_RendererStorage->WhiteTexture);
    }

    s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]->BeginRecording();
    s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]->BeginTimestamp(true);

    // todo: auto image transition layouts
    //  Clear pass
    {
        Ref<CommandBuffer> renderCommandBuffer = CommandBuffer::Create(ECommandBufferType::COMMAND_BUFFER_TYPE_GRAPHICS);
        renderCommandBuffer->BeginRecording(true);

        s_RendererStorage->SetupFramebuffer[s_RendererStorage->CurrentFrame]->BeginPass(renderCommandBuffer);
        s_RendererStorage->SetupFramebuffer[s_RendererStorage->CurrentFrame]->EndPass(renderCommandBuffer);

        renderCommandBuffer->EndRecording();
        renderCommandBuffer->Submit();
    }

    {
        s_RendererStorage->UBGlobalLighting.Gamma = s_RendererSettings.Gamma;
        for (uint32_t i = 0; i < s_RendererStorage->CurrentSpotLightIndex; ++i)
            s_RendererStorage->UBGlobalLighting.SpotLights[i] = SpotLight();

        for (uint32_t i = 0; i < s_RendererStorage->CurrentDirLightIndex; ++i)
            s_RendererStorage->UBGlobalLighting.DirLights[i] = DirectionalLight();

        for (uint32_t i = 0; i < s_RendererStorage->CurrentPointLightIndex; ++i)
            s_RendererStorage->UBGlobalLighting.PointLights[i] = PointLight();

        s_RendererStorage->CurrentPointLightIndex = 0;
        s_RendererStorage->CurrentDirLightIndex   = 0;
        s_RendererStorage->CurrentSpotLightIndex  = 0;
    }

    s_RendererStorage->SortedGeometry.clear();

    s_RendererStorage->GPUParticleSystem->OnUpdate();
}

void Renderer::Flush()
{
    // Particle System
    {
        s_RendererStorage->GPUParticleSystem->OnCompute(s_RendererSettings.ParticleCount);

        struct PushConstants
        {
            glm::mat4 CameraProjection = glm::mat4(1.0f);
            glm::mat4 CameraView       = glm::mat4(1.0f);
        } u_ParticleSystemData;

        u_ParticleSystemData.CameraProjection = s_RendererStorage->UBGlobalCamera.Projection;
        u_ParticleSystemData.CameraView       = s_RendererStorage->UBGlobalCamera.View;

        s_RendererStorage->GPUParticleSystem->OnRender(&u_ParticleSystemData);
    }
    // TODO: instead of creating new command buffer, insert execution dependency barrier!!

    // TEMP
    auto vulkanCommandBuffer =
        std::static_pointer_cast<VulkanCommandBuffer>(s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]);
    GNT_ASSERT(vulkanCommandBuffer, "Failed to cast CommandBuffer to VulkanCommandBuffer");

    vulkanCommandBuffer->InsertBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                       VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 0, nullptr);

    // SSAO-Pass
    s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]->BeginTimestamp();
    if (!s_RendererStorage->SortedGeometry.empty() && Renderer::GetSettings().AO.EnableSSAO)
    {
        s_RendererStorage->SSAODataBuffer.CameraProjection  = s_RendererStorage->UBGlobalCamera.Projection;
        s_RendererStorage->SSAODataBuffer.InvViewProjection = s_RendererStorage->UBGlobalCamera.View;
        s_RendererStorage->SSAODataBuffer.Radius            = s_RendererSettings.AO.Radius;
        s_RendererStorage->SSAODataBuffer.Bias              = s_RendererSettings.AO.Bias;
        s_RendererStorage->SSAODataBuffer.Magnitude         = s_RendererSettings.AO.Magnitude;

        // Updating SSAO
        s_RendererStorage->SSAOUniformBuffer[s_RendererStorage->CurrentFrame]->SetData(&s_RendererStorage->SSAODataBuffer, sizeof(UBSSAO));

        s_RendererStorage->SSAOPipeline->GetSpecification().Shader->Set(
            "u_PositionMap", s_RendererStorage->GeometryFramebuffer[s_RendererStorage->CurrentFrame]->GetAttachments()[0].Attachment);

        s_RendererStorage->SSAOPipeline->GetSpecification().Shader->Set(
            "u_NormalMap", s_RendererStorage->GeometryFramebuffer[s_RendererStorage->CurrentFrame]->GetAttachments()[1].Attachment);

        s_RendererStorage->SSAOPipeline->GetSpecification().Shader->Set(
            "u_UBSSAO", s_RendererStorage->SSAOUniformBuffer[s_RendererStorage->CurrentFrame]);

        s_RendererStorage->SSAOFramebuffer[s_RendererStorage->CurrentFrame]->BeginPass(
            s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]);

        SubmitFullscreenQuad(s_RendererStorage->SSAOPipeline);

        s_RendererStorage->SSAOFramebuffer[s_RendererStorage->CurrentFrame]->EndPass(
            s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]);

        if (Renderer::GetSettings().AO.BlurSSAO)
        {
            s_RendererStorage->SSAOBlurFramebuffer[s_RendererStorage->CurrentFrame]->BeginPass(
                s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]);

            // Updating SSAO-Blur
            s_RendererStorage->SSAOBlurPipeline->GetSpecification().Shader->Set(
                "u_SSAOMap", s_RendererStorage->SSAOFramebuffer[s_RendererStorage->CurrentFrame]->GetAttachments()[0].Attachment);

            SubmitFullscreenQuad(s_RendererStorage->SSAOBlurPipeline);

            s_RendererStorage->SSAOBlurFramebuffer[s_RendererStorage->CurrentFrame]->EndPass(
                s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]);
        }
    }
    s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]->EndTimestamp();

    // TEMP
    vulkanCommandBuffer->InsertBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                       VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 0, nullptr);

    // Final Lighting-Pass
    s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]->BeginTimestamp();
    {
        s_RendererStorage->LightingFramebuffer[s_RendererStorage->CurrentFrame]->BeginPass(
            s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]);

        s_RendererStorage->LightingUniformBuffer[s_RendererStorage->CurrentFrame]->SetData(&s_RendererStorage->UBGlobalLighting,
                                                                                           sizeof(s_RendererStorage->UBGlobalLighting));

        // Updating Lighting
        s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set(
            "u_PositionMap", s_RendererStorage->GeometryFramebuffer[s_RendererStorage->CurrentFrame]->GetAttachments()[0].Attachment);

        s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set(
            "u_NormalMap", s_RendererStorage->GeometryFramebuffer[s_RendererStorage->CurrentFrame]->GetAttachments()[1].Attachment);

        s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set(
            "u_AlbedoMap", s_RendererStorage->GeometryFramebuffer[s_RendererStorage->CurrentFrame]->GetAttachments()[2].Attachment);
        s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set(
            "u_MRAO", s_RendererStorage->GeometryFramebuffer[s_RendererStorage->CurrentFrame]->GetAttachments()[3].Attachment);

        s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set(
            "u_LightingData", s_RendererStorage->LightingUniformBuffer[s_RendererStorage->CurrentFrame]);

        MeshPushConstants pushConstants = {};
        pushConstants.Data              = glm::vec4(s_RendererStorage->UBGlobalCamera.Position, 0.0f);

        SubmitFullscreenQuad(s_RendererStorage->LightingPipeline, &pushConstants);

        s_RendererStorage->LightingFramebuffer[s_RendererStorage->CurrentFrame]->EndPass(
            s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]);
    }
    s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]->EndTimestamp();

    // Chromatic Aberration
    s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]->BeginTimestamp();
    {
        s_RendererStorage->ChromaticAberrationFramebuffer[s_RendererStorage->CurrentFrame]->BeginPass(
            s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]);

        // Chromatic Aberration
        s_RendererStorage->ChromaticAberrationPipeline->GetSpecification().Shader->Set(
            "u_FinalImage", s_RendererStorage->LightingFramebuffer[s_RendererStorage->CurrentFrame]->GetAttachments()[0].Attachment);

        SubmitFullscreenQuad(s_RendererStorage->ChromaticAberrationPipeline);

        s_RendererStorage->ChromaticAberrationFramebuffer[s_RendererStorage->CurrentFrame]->EndPass(
            s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]);
    }
    s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]->EndTimestamp();

    s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]->EndTimestamp(true);
    s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]->EndRecording();
    s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]->Submit();

    CollectPassStatistics();
}

void Renderer::CollectPassStatistics()
{
    auto& timestampResults = s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]->GetTimestampResults();

    {
        const float time =
            static_cast<float>(timestampResults[1] - timestampResults[0]) * GraphicsContext::Get().GetTimestampPeriod() / 1000000.0f;
        const std::string str = "DirShadowMap Pass: " + std::to_string(time) + " (ms)";
        s_RendererStats.PassStatistsics.push_back(str);
    }

    {
        const float time =
            static_cast<float>(timestampResults[3] - timestampResults[2]) * GraphicsContext::Get().GetTimestampPeriod() / 1000000.0f;
        const std::string str = "GBuffer Pass:   " + std::to_string(time) + " (ms)";
        s_RendererStats.PassStatistsics.push_back(str);
    }

    {
        const float time =
            static_cast<float>(timestampResults[5] - timestampResults[4]) * GraphicsContext::Get().GetTimestampPeriod() / 1000000.0f;
        const std::string str = "SSAO Pass:      " + std::to_string(time) + " (ms)";
        s_RendererStats.PassStatistsics.push_back(str);
    }

    {
        const float time =
            static_cast<float>(timestampResults[7] - timestampResults[6]) * GraphicsContext::Get().GetTimestampPeriod() / 1000000.0f;
        const std::string str = "Lighting Pass:  " + std::to_string(time) + " (ms)";
        s_RendererStats.PassStatistsics.push_back(str);
    }

    {
        const float time =
            static_cast<float>(timestampResults[9] - timestampResults[8]) * GraphicsContext::Get().GetTimestampPeriod() / 1000000.0f;
        const std::string str = "Chromatic Aberration: " + std::to_string(time) + " (ms)";
        s_RendererStats.PassStatistsics.push_back(str);
    }
}

void Renderer::BeginScene(const Camera& camera)
{
    s_RendererStorage->UBGlobalCamera.Projection = camera.GetProjectionMatrix();
    s_RendererStorage->UBGlobalCamera.View       = camera.GetViewMatrix();
    s_RendererStorage->UBGlobalCamera.Position   = camera.GetPosition();

    s_RendererStorage->CameraUniformBuffer[s_RendererStorage->CurrentFrame]->SetData(&s_RendererStorage->UBGlobalCamera, sizeof(UBCamera));
    Renderer2D::GetStorageData().CameraProjectionMatrix = camera.GetViewProjectionMatrix();
}

const std::vector<std::string> Renderer::GetPipelineStatistics()
{
    std::vector<std::string> result =
        s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]->GetPipelineStatisticsStrings();
    auto& statisticsResults = s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]->GetPipelineStatisticsResults();

    for (uint32_t i = 0; i < s_RendererStats.PipelineStatisticsResults.size(); ++i)
    {
        s_RendererStats.PipelineStatisticsResults[i] += statisticsResults[i];
        result[i] += std::to_string(s_RendererStats.PipelineStatisticsResults[i]);
    }

    return result;
}

void Renderer::EndScene()
{
    std::sort(s_RendererStorage->SortedGeometry.begin(), s_RendererStorage->SortedGeometry.end(),
              [&](const GeometryData& lhs, const GeometryData& rhs)
              {
                  const glm::vec3& cameraPos = s_RendererStorage->UBGlobalCamera.Position;
                  const glm::vec3 lhsPos(lhs.Transform[3]);
                  const glm::vec3 rhsPos(rhs.Transform[3]);

                  const float lhsLength = glm::length(cameraPos - lhsPos);
                  const float rhsLength = glm::length(cameraPos - rhsPos);

                  return lhsLength > rhsLength;  // Descending order
              });

    // TODO: Compute Culling-Pass

    s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]->BeginTimestamp();
    // ShadowMap-Pass
    {
        s_RendererStorage->ShadowMapFramebuffer[s_RendererStorage->CurrentFrame]->BeginPass(
            s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]);

        if (Renderer::GetSettings().Shadows.RenderShadows)
        {
            constexpr float cameraWidth     = 12.0f;
            constexpr float zNear           = 0.0000001f;
            constexpr float zFar            = 1024.0f;
            const glm::mat4 lightProjection = glm::ortho(-cameraWidth, cameraWidth, -cameraWidth, cameraWidth, zNear, zFar);

            const glm::mat4 lightView = glm::lookAt(glm::vec3(s_RendererStorage->UBGlobalLighting.DirLights[0].Direction), glm::vec3(0.0f),
                                                    glm::vec3(0.0f, 1.0f, 0.0f));

            s_RendererStorage->MeshShadowsBuffer.LightSpaceMatrix = lightProjection * lightView;

            MatrixPushConstants mpc = {};
            mpc.mat2                = s_RendererStorage->MeshShadowsBuffer.LightSpaceMatrix;
            for (auto& geometry : s_RendererStorage->SortedGeometry)
            {
                mpc.mat1 = geometry.Transform;
                SubmitMesh(s_RendererStorage->ShadowMapPipeline, geometry.VertexBuffer, geometry.IndexBuffer, nullptr, &mpc);
            }
        }
        s_RendererStorage->ShadowMapFramebuffer[s_RendererStorage->CurrentFrame]->EndPass(
            s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]);
    }
    s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]->EndTimestamp();

    // GPass
    s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]->BeginTimestamp();
    {
        s_RendererStorage->GeometryFramebuffer[s_RendererStorage->CurrentFrame]->BeginPass(
            s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]);

        for (auto& geometry : s_RendererStorage->SortedGeometry)
        {
            MatrixPushConstants mpc = {};
            mpc.mat1                = geometry.Transform;
            mpc.mat2                = glm::mat4(glm::transpose(glm::inverse(glm::mat3(geometry.Transform))));
            geometry.Material->Update();  // Is it useless?

#if MESH_SHADING_TEST
            SubmitMeshShading();
#else
            SubmitMesh(s_RendererStorage->GeometryPipeline, geometry.VertexBuffer, geometry.IndexBuffer, geometry.Material, &mpc);
#endif
        }

        s_RendererStorage->GeometryFramebuffer[s_RendererStorage->CurrentFrame]->EndPass(
            s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]);
    }
    s_RendererStorage->RenderCommandBuffer[s_RendererStorage->CurrentFrame]->EndTimestamp();

    /*  // PBR-ForwardPass
    {
        BeginRenderPass(s_RendererStorage->PBRFramebuffer, glm::vec4(0.5f, 0.0f, 0.0f, 1.0f));

        for (auto& geometry : s_RendererStorage->SortedGeometry)
        {
            MeshPushConstants pushConstants = {};
            pushConstants.TransformMatrix   = geometry.Transform;

            SubmitMesh(s_RendererStorage->PBRPipeline, geometry.VertexBuffer, geometry.IndexBuffer, geometry.Material, &pushConstants);
        }

        EndRenderPass(s_RendererStorage->PBRFramebuffer);
    }*/
}

void Renderer::AddPointLight(const glm::vec3& position, const glm::vec3& color, const float intensity, int32_t active)
{
    if (s_RendererStorage->CurrentPointLightIndex >= s_MAX_POINT_LIGHTS) return;

    s_RendererStorage->UBGlobalLighting.PointLights[s_RendererStorage->CurrentPointLightIndex].Position  = glm::vec4(position, 0.0f);
    s_RendererStorage->UBGlobalLighting.PointLights[s_RendererStorage->CurrentPointLightIndex].Color     = glm::vec4(color, 0.0f);
    s_RendererStorage->UBGlobalLighting.PointLights[s_RendererStorage->CurrentPointLightIndex].Intensity = intensity;

    const bool bIsActive = color.x > 0.0f || color.y > 0.0f || color.z > 0.0f;
    s_RendererStorage->UBGlobalLighting.PointLights[s_RendererStorage->CurrentPointLightIndex].IsActive = active;
    ++s_RendererStorage->CurrentPointLightIndex;
}

void Renderer::AddDirectionalLight(const glm::vec3& color, const glm::vec3& direction, int32_t castShadows, float intensity)
{
    s_RendererStorage->UBGlobalLighting.Gamma = s_RendererSettings.Gamma;

    if (s_RendererStorage->CurrentDirLightIndex >= s_MAX_DIR_LIGHTS) return;

    s_RendererStorage->UBGlobalLighting.DirLights[s_RendererStorage->CurrentDirLightIndex].Color       = glm::vec4(color, 0.0f);
    s_RendererStorage->UBGlobalLighting.DirLights[s_RendererStorage->CurrentDirLightIndex].Direction   = glm::vec4(direction, 0.0f);
    s_RendererStorage->UBGlobalLighting.DirLights[s_RendererStorage->CurrentDirLightIndex].CastShadows = castShadows;
    s_RendererStorage->UBGlobalLighting.DirLights[s_RendererStorage->CurrentDirLightIndex].Intensity   = intensity;
    ++s_RendererStorage->CurrentDirLightIndex;
}

void Renderer::AddSpotLight(const glm::vec3& position, const glm::vec3& direction, const glm::vec3& color, const float intensity,
                            const int32_t active, const float cutOff, const float outerCutOff)
{
    if (s_RendererStorage->CurrentSpotLightIndex >= s_MAX_SPOT_LIGHTS) return;

    s_RendererStorage->UBGlobalLighting.SpotLights[s_RendererStorage->CurrentSpotLightIndex].Position    = glm::vec4(position, 0.0f);
    s_RendererStorage->UBGlobalLighting.SpotLights[s_RendererStorage->CurrentSpotLightIndex].Color       = glm::vec4(color, 0.0f);
    s_RendererStorage->UBGlobalLighting.SpotLights[s_RendererStorage->CurrentSpotLightIndex].Direction   = glm::vec4(direction, 0.0f);
    s_RendererStorage->UBGlobalLighting.SpotLights[s_RendererStorage->CurrentSpotLightIndex].CutOff      = cutOff;
    s_RendererStorage->UBGlobalLighting.SpotLights[s_RendererStorage->CurrentSpotLightIndex].OuterCutOff = outerCutOff;

    s_RendererStorage->UBGlobalLighting.SpotLights[s_RendererStorage->CurrentSpotLightIndex].Intensity = intensity;
    s_RendererStorage->UBGlobalLighting.SpotLights[s_RendererStorage->CurrentSpotLightIndex].IsActive  = active;

    ++s_RendererStorage->CurrentSpotLightIndex;
}

void Renderer::SubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& transform)
{
    for (uint32_t i = 0; i < mesh->GetSubmeshCount(); ++i)
    {
#if MESH_SHADING_TEST
        s_RendererStorage->SortedGeometry.emplace_back(mesh->GetMaterial(i), mesh->GetVertexBuffers()[i], mesh->GetIndexBuffers()[i],
                                                       mesh->GetMeshletBuffers()[i], mesh->GetMeshletSize() transform);
#else
        s_RendererStorage->SortedGeometry.emplace_back(mesh->GetMaterial(i), mesh->GetVertexBuffers()[i], mesh->GetIndexBuffers()[i],
                                                       transform);
#endif
    }
}

std::vector<RendererOutput> Renderer::GetRendererOutput()
{
    std::vector<RendererOutput> rendererOutput;

    rendererOutput.emplace_back(s_RendererStorage->ShadowMapFramebuffer[s_RendererStorage->CurrentFrame]->GetAttachments()[0].Attachment,
                                "ShadowMap");
    rendererOutput.emplace_back(s_RendererStorage->GeometryFramebuffer[s_RendererStorage->CurrentFrame]->GetAttachments()[0].Attachment,
                                "Position");
    rendererOutput.emplace_back(s_RendererStorage->GeometryFramebuffer[s_RendererStorage->CurrentFrame]->GetAttachments()[1].Attachment,
                                "Normal");
    rendererOutput.emplace_back(s_RendererStorage->GeometryFramebuffer[s_RendererStorage->CurrentFrame]->GetAttachments()[2].Attachment,
                                "Albedo");
    rendererOutput.emplace_back(s_RendererStorage->GeometryFramebuffer[s_RendererStorage->CurrentFrame]->GetAttachments()[3].Attachment,
                                "Depth");
    rendererOutput.emplace_back(s_RendererStorage->SSAOFramebuffer[s_RendererStorage->CurrentFrame]->GetAttachments()[0].Attachment,
                                "SSAO");
    rendererOutput.emplace_back(s_RendererStorage->SSAOBlurFramebuffer[s_RendererStorage->CurrentFrame]->GetAttachments()[0].Attachment,
                                "SSAO-Blur");
    rendererOutput.emplace_back(s_RendererStorage->SSAONoiseTexture->GetImage(), "SSAO-Noise");
    rendererOutput.emplace_back(
        s_RendererStorage->ChromaticAberrationFramebuffer[s_RendererStorage->CurrentFrame]->GetAttachments()[0].Attachment,
        "ChromaticAberration");

    return rendererOutput;
}

}  // namespace Gauntlet