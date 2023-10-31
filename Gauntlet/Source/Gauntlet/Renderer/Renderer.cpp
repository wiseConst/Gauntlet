#include "GauntletPCH.h"
#include "Renderer.h"
#include "Shader.h"
#include "Texture.h"
#include "Pipeline.h"
#include "Framebuffer.h"
#include "Mesh.h"
#include "Camera/Camera.h"

#include "Renderer2D.h"
#include "GraphicsContext.h"

#include "Gauntlet/Core/Random.h"
#include "Animation.h"

#include "Gauntlet/Platform/Vulkan/VulkanRenderer.h"

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

    s_RendererStorage->CameraUniformBuffer = UniformBuffer::Create(sizeof(UBCamera));
    s_RendererStorage->CameraUniformBuffer->MapPersistent();
    {
        const uint32_t WhiteTexutreData = 0xffffffff;
        s_RendererStorage->WhiteTexture = Texture2D::Create(&WhiteTexutreData, sizeof(WhiteTexutreData), 1, 1);
    }

    // Geometry pipeline
    {
        FramebufferSpecification geometryFramebufferSpec = {};

        FramebufferAttachmentSpecification attachment = {};
        attachment.LoadOp                             = ELoadOp::LOAD;
        attachment.StoreOp                            = EStoreOp::STORE;
        attachment.ClearColor                         = glm::vec4(0.0f);
        attachment.Filter                             = ETextureFilter::LINEAR;
        //        attachment.Wrap                               = ETextureWrap::CLAMP;

        // Position
        attachment.Format = EImageFormat::RGBA16F;
        geometryFramebufferSpec.Attachments.push_back(attachment);

        // Normal
        attachment.Format = EImageFormat::RGBA16F;
        geometryFramebufferSpec.Attachments.push_back(attachment);

        // attachment.Wrap = ETextureWrap::REPEAT;

        // Albedo
        attachment.Format = EImageFormat::RGBA;
        geometryFramebufferSpec.Attachments.push_back(attachment);

        // Depth
        attachment.ClearColor = glm::vec4(1.0f, glm::vec3(0.0f));
        attachment.Format     = EImageFormat::DEPTH32F;
        geometryFramebufferSpec.Attachments.push_back(attachment);

        geometryFramebufferSpec.Name = "GeometryDeferred";

        s_RendererStorage->GeometryFramebuffer = Framebuffer::Create(geometryFramebufferSpec);

        auto GeometryShader                       = ShaderLibrary::Load("Resources/Cached/Shaders/Geometry");
        s_RendererStorage->MeshVertexBufferLayout = GeometryShader->GetVertexBufferLayout();

        PipelineSpecification geometryPipelineSpec = {};
        geometryPipelineSpec.Name                  = "Geometry";
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
        shadowMapAttachment.Wrap                               = ETextureWrap::CLAMP_TO_BORDER;

        shadowMapFramebufferSpec.Attachments = {shadowMapAttachment};
        shadowMapFramebufferSpec.Name        = "ShadowMap";
        shadowMapFramebufferSpec.Width       = 2048;
        shadowMapFramebufferSpec.Height      = 2048;

        s_RendererStorage->ShadowMapFramebuffer = Framebuffer::Create(shadowMapFramebufferSpec);

        PipelineSpecification shadowmapPipelineSpec = {};
        shadowmapPipelineSpec.Name                  = "ShadowMap";
        shadowmapPipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        shadowmapPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        shadowmapPipelineSpec.CullMode              = ECullMode::CULL_MODE_FRONT;
        shadowmapPipelineSpec.bDepthWrite           = true;
        shadowmapPipelineSpec.bDepthTest            = true;
        shadowmapPipelineSpec.DepthCompareOp        = ECompareOp::COMPARE_OP_LESS;
        shadowmapPipelineSpec.TargetFramebuffer     = s_RendererStorage->ShadowMapFramebuffer;
        shadowmapPipelineSpec.Layout                = s_RendererStorage->MeshVertexBufferLayout;
        shadowmapPipelineSpec.Shader                = ShaderLibrary::Load("Resources/Cached/Shaders/Depth");

        s_RendererStorage->ShadowMapPipeline    = Pipeline::Create(shadowmapPipelineSpec);
        s_RendererStorage->ShadowsUniformBuffer = UniformBuffer::Create(sizeof(UBShadows));
        s_RendererStorage->ShadowsUniformBuffer->MapPersistent();
    }

    // Forward
    {
        PipelineSpecification forwardPipelineSpec = {};
        forwardPipelineSpec.Name                  = "ForwardPass";
        forwardPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        forwardPipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        forwardPipelineSpec.PrimitiveTopology     = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        forwardPipelineSpec.CullMode              = ECullMode::CULL_MODE_NONE;
        forwardPipelineSpec.bDepthTest            = true;
        forwardPipelineSpec.bDepthWrite           = true;
        forwardPipelineSpec.DepthCompareOp        = ECompareOp::COMPARE_OP_LESS;
        forwardPipelineSpec.bDynamicPolygonMode   = true;
        forwardPipelineSpec.TargetFramebuffer     = s_RendererStorage->GeometryFramebuffer;
        forwardPipelineSpec.Shader                = ShaderLibrary::Load("Resources/Cached/Shaders/Forward");

        s_RendererStorage->ForwardPassPipeline = Pipeline::Create(forwardPipelineSpec);
    }

    // SSAO
    {
        FramebufferSpecification ssaoFramebufferSpec = {};
        ssaoFramebufferSpec.Name                     = "SSAO";

        FramebufferAttachmentSpecification ssaoFramebufferAttachmentSpec = {};
        ssaoFramebufferAttachmentSpec.Format                             = EImageFormat::R32F;
        ssaoFramebufferAttachmentSpec.Filter                             = ETextureFilter::LINEAR;
        ssaoFramebufferSpec.Attachments                                  = {ssaoFramebufferAttachmentSpec};

        s_RendererStorage->SSAOFramebuffer = Framebuffer::Create(ssaoFramebufferSpec);

        PipelineSpecification ssaoPipelineSpec = {};
        ssaoPipelineSpec.Name                  = "SSAO";
        ssaoPipelineSpec.Shader                = ShaderLibrary::Load("Resources/Cached/Shaders/SSAO");
        ssaoPipelineSpec.CullMode              = ECullMode::CULL_MODE_BACK;
        ssaoPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        ssaoPipelineSpec.TargetFramebuffer     = s_RendererStorage->SSAOFramebuffer;

        s_RendererStorage->SSAOPipeline      = Pipeline::Create(ssaoPipelineSpec);
        s_RendererStorage->SSAOUniformBuffer = UniformBuffer::Create(sizeof(UBSSAO));
        s_RendererStorage->SSAOUniformBuffer->MapPersistent();
    }

    // SSAO-Blur
    {
        FramebufferSpecification ssaoBlurFramebufferSpec = {};
        ssaoBlurFramebufferSpec.Name                     = "SSAO-Blur";

        FramebufferAttachmentSpecification ssaoBlurFramebufferAttachmentSpec = {};
        ssaoBlurFramebufferAttachmentSpec.Format                             = EImageFormat::R32F;
        ssaoBlurFramebufferAttachmentSpec.Filter                             = ETextureFilter::NEAREST;
        ssaoBlurFramebufferSpec.Attachments                                  = {ssaoBlurFramebufferAttachmentSpec};

        s_RendererStorage->SSAOBlurFramebuffer = Framebuffer::Create(ssaoBlurFramebufferSpec);

        PipelineSpecification ssaoBlurPipelineSpec = {};
        ssaoBlurPipelineSpec.Name                  = "SSAO-Blur";
        ssaoBlurPipelineSpec.Shader                = ShaderLibrary::Load("Resources/Cached/Shaders/SSAO-Blur");
        ssaoBlurPipelineSpec.CullMode              = ECullMode::CULL_MODE_BACK;
        ssaoBlurPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        ssaoBlurPipelineSpec.TargetFramebuffer     = s_RendererStorage->SSAOBlurFramebuffer;

        s_RendererStorage->SSAOBlurPipeline = Pipeline::Create(ssaoBlurPipelineSpec);

        for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            s_RendererStorage->SSAOBlurPipeline->GetSpecification().Shader->Set(
                "u_SSAOMap", s_RendererStorage->SSAOFramebuffer->GetAttachments()[0].Attachments[frame]);
        }

        // Generating the kernel(random points in hemisphere: z [0, 1]) we use them in shader for sampling depth
        for (uint32_t i = 0; i < 16; ++i)
        {
            glm::vec3 sample(Random::GetInRange(0.0f, 1.0f) * 2.0 - 1.0, Random::GetInRange(0.0f, 1.0f) * 2.0 - 1.0,
                             Random::GetInRange(0.0f, 1.0f));
            sample *= Random::GetInRange(0.0f, 1.0f);
            sample = glm::normalize(sample);

            // Scale sample because we want them to be dense to the origin
            float scale = (float)i / 16.0f;
            scale       = Math::Lerp(0.1f, 1.0f, scale * scale);
            sample *= scale;
            s_RendererStorage->SSAODataBuffer.Samples[i] = glm::vec4(sample, 0.0f);
        }

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
        s_RendererStorage->SSAONoiseTexture       = Texture2D::Create(ssaoNoise.data(), ssaoNoise.size() * sizeof(ssaoNoise[0]), 4, 4);
    }

    // Lighting
    {
        FramebufferSpecification lightingFramebufferSpec = {};

        FramebufferAttachmentSpecification attachment = {};
        attachment.ClearColor                         = glm::vec4(glm::vec3(0.1f), 1.0f);
        attachment.Format                             = EImageFormat::R11G11B10;
        attachment.LoadOp                             = ELoadOp::CLEAR;
        attachment.StoreOp                            = EStoreOp::STORE;
        lightingFramebufferSpec.Attachments.push_back(attachment);

        lightingFramebufferSpec.Name           = "LightingDeferred";
        s_RendererStorage->LightingFramebuffer = Framebuffer::Create(lightingFramebufferSpec);

        PipelineSpecification lightingPipelineSpec = {};
        lightingPipelineSpec.Name                  = "Lighting";
        lightingPipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        lightingPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        lightingPipelineSpec.TargetFramebuffer     = s_RendererStorage->LightingFramebuffer;
        lightingPipelineSpec.Shader                = ShaderLibrary::Load("Resources/Cached/Shaders/Lighting");

        s_RendererStorage->LightingPipeline = Pipeline::Create(lightingPipelineSpec);

        s_RendererStorage->LightingUniformBuffer = UniformBuffer::Create(sizeof(UBBlinnPhong));
        s_RendererStorage->LightingUniformBuffer->MapPersistent();
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

        s_RendererStorage->ChromaticAberrationFramebuffer = Framebuffer::Create(caFramebufferSpec);

        PipelineSpecification caPipelineSpec = {};
        caPipelineSpec.Name                  = "ChromaticAbberation";
        caPipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        caPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        caPipelineSpec.TargetFramebuffer     = s_RendererStorage->ChromaticAberrationFramebuffer;
        caPipelineSpec.Shader                = ShaderLibrary::Load("Resources/Cached/Shaders/ChromaticAberration");

        s_RendererStorage->ChromaticAberrationPipeline = Pipeline::Create(caPipelineSpec);
    }

    // Chromatic Aberration
    {
        FramebufferSpecification pixelFramebufferSpec = {};
        FramebufferAttachmentSpecification attachment = {};
        attachment.ClearColor                         = glm::vec4(0.0f);
        attachment.Format                             = EImageFormat::R11G11B10;
        attachment.LoadOp                             = ELoadOp::CLEAR;
        attachment.StoreOp                            = EStoreOp::STORE;
        pixelFramebufferSpec.Attachments.push_back(attachment);
        pixelFramebufferSpec.Name = "Pixelization";

        s_RendererStorage->PixelizationFramebuffer = Framebuffer::Create(pixelFramebufferSpec);

        PipelineSpecification pixelPipelineSpec = {};
        pixelPipelineSpec.Name                  = "Pixelization";
        pixelPipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        pixelPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        pixelPipelineSpec.TargetFramebuffer     = s_RendererStorage->PixelizationFramebuffer;
        pixelPipelineSpec.Shader                = ShaderLibrary::Load("Resources/Cached/Shaders/Pixelization");

        s_RendererStorage->PixelizationPipeline = Pipeline::Create(pixelPipelineSpec);
    }

    // Clear-Pass
    {
        FramebufferSpecification setupFramebufferSpec = {};
        setupFramebufferSpec.ExistingAttachments      = s_RendererStorage->GeometryFramebuffer->GetAttachments();
        setupFramebufferSpec.Name                     = "Setup";

        s_RendererStorage->SetupFramebuffer = Framebuffer::Create(setupFramebufferSpec);
    }

    for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
    {
        // Pixelization
        s_RendererStorage->PixelizationPipeline->GetSpecification().Shader->Set(
            "u_FinalImage", s_RendererStorage->LightingFramebuffer->GetAttachments()[0].Attachments[frame]);

        // Chromatic Aberration
        s_RendererStorage->ChromaticAberrationPipeline->GetSpecification().Shader->Set(
            "u_FinalImage", s_RendererStorage->LightingFramebuffer->GetAttachments()[0].Attachments[frame]);

        // SSAO
        s_RendererStorage->SSAOPipeline->GetSpecification().Shader->Set(
            "u_PositionMap", s_RendererStorage->GeometryFramebuffer->GetAttachments()[0].Attachments[frame]);

        s_RendererStorage->SSAOPipeline->GetSpecification().Shader->Set(
            "u_NormalMap", s_RendererStorage->GeometryFramebuffer->GetAttachments()[1].Attachments[frame]);

        s_RendererStorage->SSAOPipeline->GetSpecification().Shader->Set("u_TexNoiseMap", s_RendererStorage->SSAONoiseTexture);

        s_RendererStorage->SSAOPipeline->GetSpecification().Shader->Set("u_UBSSAO", s_RendererStorage->SSAOUniformBuffer, 0);

        // Lighting
        s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set(
            "u_PositionMap", s_RendererStorage->GeometryFramebuffer->GetAttachments()[0].Attachments[frame]);

        s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set(
            "u_NormalMap", s_RendererStorage->GeometryFramebuffer->GetAttachments()[1].Attachments[frame]);

        s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set(
            "u_AlbedoMap", s_RendererStorage->GeometryFramebuffer->GetAttachments()[2].Attachments[frame]);

        s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set(
            "u_ShadowMap", s_RendererStorage->ShadowMapFramebuffer->GetAttachments()[0].Attachments[frame]);
        s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set(
            "u_SSAOMap", s_RendererStorage->SSAOBlurFramebuffer->GetAttachments()[0].Attachments[frame]);

        s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set("u_LightingModelBuffer",
                                                                            s_RendererStorage->LightingUniformBuffer, 0);
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

        s_RendererStorage->PBRFramebuffer = Framebuffer::Create(pbrForwardFramebufferSpec);

        auto PBRShader = ShaderLibrary::Load("Resources/Cached/Shaders/PBR");

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

    s_Renderer->PostInit();

    s_RendererStorage->UploadHeap = StagingBuffer::Create(10240);  // 10 KB
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

    s_RendererStorage->UploadHeap->Destroy();

    s_RendererStorage->GeometryFramebuffer->Destroy();
    s_RendererStorage->GeometryPipeline->Destroy();

    s_RendererStorage->SetupFramebuffer->Destroy();

    s_RendererStorage->ForwardPassPipeline->Destroy();
    //  s_RendererStorage->AnimationPipeline->Destroy();

    // testing
    s_RendererStorage->PBRFramebuffer->Destroy();
    s_RendererStorage->PBRPipeline->Destroy();

    s_RendererStorage->ShadowMapFramebuffer->Destroy();
    s_RendererStorage->ShadowMapPipeline->Destroy();
    s_RendererStorage->ShadowsUniformBuffer->Destroy();

    s_RendererStorage->SSAOFramebuffer->Destroy();
    s_RendererStorage->SSAOPipeline->Destroy();
    s_RendererStorage->SSAOUniformBuffer->Destroy();

    s_RendererStorage->SSAONoiseTexture->Destroy();
    s_RendererStorage->SSAOBlurFramebuffer->Destroy();
    s_RendererStorage->SSAOBlurPipeline->Destroy();

    s_RendererStorage->LightingFramebuffer->Destroy();
    s_RendererStorage->LightingPipeline->Destroy();
    s_RendererStorage->LightingUniformBuffer->Destroy();

    s_RendererStorage->ChromaticAberrationFramebuffer->Destroy();
    s_RendererStorage->ChromaticAberrationPipeline->Destroy();

    s_RendererStorage->PixelizationFramebuffer->Destroy();
    s_RendererStorage->PixelizationPipeline->Destroy();

    s_RendererStorage->CameraUniformBuffer->Destroy();

    s_RendererStorage->WhiteTexture->Destroy();
    delete s_RendererStorage;
    s_RendererStorage = nullptr;

    delete s_Renderer;
    s_Renderer = nullptr;
}

const Ref<Image>& Renderer::GetFinalImage()
{
    return s_RendererStorage->PBRFramebuffer->GetAttachments()[0].Attachments[GraphicsContext::Get().GetCurrentFrameIndex()];
    /* if (s_RendererSettings.ChromaticAberrationView)
         return s_RendererStorage->ChromaticAberrationFramebuffer->GetAttachments()[0]
             .Attachments[GraphicsContext::Get().GetCurrentFrameIndex()];
     if (s_RendererSettings.PixelatedView)
         return s_RendererStorage->PixelizationFramebuffer->GetAttachments()[0].Attachments[GraphicsContext::Get().GetCurrentFrameIndex()];

     return s_RendererStorage->LightingFramebuffer->GetAttachments()[0].Attachments[GraphicsContext::Get().GetCurrentFrameIndex()];*/
}

void Renderer::Begin()
{
    s_Renderer->BeginImpl();
    Renderer::GetStats().DrawCalls = 0;

    // Empty pipeline stats
    for (auto& pipelineStat : s_PipelineStats)
        pipelineStat = 0;

    if (s_RendererStorage->bFramebuffersNeedResize)
    {
        s_RendererStorage->GeometryFramebuffer->Resize(s_RendererStorage->NewFramebufferSize.x, s_RendererStorage->NewFramebufferSize.y);
        s_RendererStorage->SetupFramebuffer->Resize(s_RendererStorage->NewFramebufferSize.x, s_RendererStorage->NewFramebufferSize.y);
        s_RendererStorage->SSAOFramebuffer->Resize(s_RendererStorage->NewFramebufferSize.x, s_RendererStorage->NewFramebufferSize.y);
        s_RendererStorage->SSAOBlurFramebuffer->Resize(s_RendererStorage->NewFramebufferSize.x, s_RendererStorage->NewFramebufferSize.y);
        s_RendererStorage->LightingFramebuffer->Resize(s_RendererStorage->NewFramebufferSize.x, s_RendererStorage->NewFramebufferSize.y);
        s_RendererStorage->ChromaticAberrationFramebuffer->Resize(s_RendererStorage->NewFramebufferSize.x,
                                                                  s_RendererStorage->NewFramebufferSize.y);
        s_RendererStorage->PixelizationFramebuffer->Resize(s_RendererStorage->NewFramebufferSize.x,
                                                           s_RendererStorage->NewFramebufferSize.y);

        s_RendererStorage->PBRFramebuffer->Resize(s_RendererStorage->NewFramebufferSize.x, s_RendererStorage->NewFramebufferSize.y);

        /*for (auto& geometry : s_Data.SortedGeometry)
        {
            geometry.Material->Invalidate();
        }*/

        for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            // Updating LightingSet
            s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set(
                "u_PositionMap", s_RendererStorage->GeometryFramebuffer->GetAttachments()[0].Attachments[frame]);

            s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set(
                "u_NormalMap", s_RendererStorage->GeometryFramebuffer->GetAttachments()[1].Attachments[frame]);

            s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set(
                "u_AlbedoMap", s_RendererStorage->GeometryFramebuffer->GetAttachments()[2].Attachments[frame]);

            s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set(
                "u_ShadowMap", s_RendererStorage->ShadowMapFramebuffer->GetAttachments()[0].Attachments[frame]);
            s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set(
                "u_SSAOMap", Renderer::GetSettings().BlurSSAO
                                 ? s_RendererStorage->SSAOBlurFramebuffer->GetAttachments()[0].Attachments[frame]
                                 : s_RendererStorage->SSAOFramebuffer->GetAttachments()[0].Attachments[frame]);

            s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set("u_LightingModelBuffer",
                                                                                s_RendererStorage->LightingUniformBuffer, 0);

            // Updating SSAO Set
            s_RendererStorage->SSAOPipeline->GetSpecification().Shader->Set(
                "u_PositionMap", s_RendererStorage->GeometryFramebuffer->GetAttachments()[0].Attachments[frame]);

            s_RendererStorage->SSAOPipeline->GetSpecification().Shader->Set(
                "u_NormalMap", s_RendererStorage->GeometryFramebuffer->GetAttachments()[1].Attachments[frame]);

            s_RendererStorage->SSAOPipeline->GetSpecification().Shader->Set("u_TexNoiseMap", s_RendererStorage->SSAONoiseTexture);

            s_RendererStorage->SSAOPipeline->GetSpecification().Shader->Set("u_UBSSAO", s_RendererStorage->SSAOUniformBuffer, 0);

            // Updating SSAO-Blur Set
            s_RendererStorage->SSAOBlurPipeline->GetSpecification().Shader->Set(
                "u_SSAOMap", s_RendererStorage->SSAOFramebuffer->GetAttachments()[0].Attachments[frame]);

            // Chromatic Aberration
            s_RendererStorage->ChromaticAberrationPipeline->GetSpecification().Shader->Set(
                "u_FinalImage", s_RendererStorage->LightingFramebuffer->GetAttachments()[0].Attachments[frame]);

            // Pixelization
            s_RendererStorage->PixelizationPipeline->GetSpecification().Shader->Set(
                "u_FinalImage", s_RendererStorage->LightingFramebuffer->GetAttachments()[0].Attachments[frame]);
        }

        s_RendererStorage->bFramebuffersNeedResize = false;
    }

    // SSAO Enable
    for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
    {
        if (Renderer::GetSettings().EnableSSAO)
            s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set(
                "u_SSAOMap", Renderer::GetSettings().BlurSSAO
                                 ? s_RendererStorage->SSAOBlurFramebuffer->GetAttachments()[0].Attachments[frame]
                                 : s_RendererStorage->SSAOFramebuffer->GetAttachments()[0].Attachments[frame]);
        else
            s_RendererStorage->LightingPipeline->GetSpecification().Shader->Set("u_SSAOMap", s_RendererStorage->WhiteTexture);
    }

    s_Renderer->BeginQuery();

    // Clear geometry buffer contents
    BeginRenderPass(s_RendererStorage->SetupFramebuffer, glm::vec4(0.3f, 0.56f, 0.841f, 1.0f));
    EndRenderPass(s_RendererStorage->SetupFramebuffer);

    {
        s_RendererStorage->UBGlobalLighting.Gamma = s_RendererSettings.Gamma;
        for (uint32_t i = 0; i < s_RendererStorage->CurrentDirLightIndex; ++i)
            s_RendererStorage->UBGlobalLighting.DirLights[i] = DirectionalLight();

        for (uint32_t i = 0; i < s_RendererStorage->CurrentPointLightIndex; ++i)
            s_RendererStorage->UBGlobalLighting.PointLights[i] = PointLight();
    }
    s_RendererStorage->CurrentPointLightIndex = 0;
    s_RendererStorage->CurrentDirLightIndex   = 0;
    s_RendererStorage->CurrentSpotLightIndex  = 0;

    s_RendererStorage->SortedGeometry.clear();
}

void Renderer::Flush()
{
    // SSAO-Pass
    if (!s_RendererStorage->SortedGeometry.empty() && Renderer::GetSettings().EnableSSAO)
    {
        static constexpr auto noiseFactor                         = 4.0f;
        s_RendererStorage->SSAODataBuffer.ViewportSizeNoiseFactor = glm::vec4(s_RendererStorage->NewFramebufferSize, noiseFactor, 0.0f);
        s_RendererStorage->SSAODataBuffer.CameraProjection        = s_RendererStorage->UBGlobalCamera.Projection;
        s_RendererStorage->SSAODataBuffer.RadiusBias = glm::vec4(s_RendererSettings.Radius, s_RendererSettings.Bias, 0.0f, 0.0f);

        s_RendererStorage->SSAOUniformBuffer->Update(&s_RendererStorage->SSAODataBuffer, sizeof(UBSSAO));

        BeginRenderPass(s_RendererStorage->SSAOFramebuffer, glm::vec4(glm::vec3(0.5f), 1.0f));

        SubmitFullscreenQuad(s_RendererStorage->SSAOPipeline);

        EndRenderPass(s_RendererStorage->SSAOFramebuffer);

        if (Renderer::GetSettings().BlurSSAO)
        {
            BeginRenderPass(s_RendererStorage->SSAOBlurFramebuffer, glm::vec4(glm::vec3(0.8f), 1.0f));

            SubmitFullscreenQuad(s_RendererStorage->SSAOBlurPipeline);

            EndRenderPass(s_RendererStorage->SSAOBlurFramebuffer);
        }
    }

    // Final Lighting-Pass
    {
        BeginRenderPass(s_RendererStorage->LightingFramebuffer, glm::vec4(0.7f, 0.7f, 0.0f, 1.0f));

        MeshPushConstants pushConstants = {};
        pushConstants.Data              = glm::vec4(s_RendererStorage->UBGlobalCamera.Position, 0.0f);
        pushConstants.TransformMatrix   = s_RendererStorage->MeshShadowsBuffer.LightSpaceMatrix;

        SubmitFullscreenQuad(s_RendererStorage->LightingPipeline, &pushConstants);

        EndRenderPass(s_RendererStorage->LightingFramebuffer);
    }

    // Chromatic Aberration
    {
        BeginRenderPass(s_RendererStorage->ChromaticAberrationFramebuffer, glm::vec4(0.23f, 0.32f, 0.0f, 1.0f));

        SubmitFullscreenQuad(s_RendererStorage->ChromaticAberrationPipeline);

        EndRenderPass(s_RendererStorage->ChromaticAberrationFramebuffer);
    }

    // Pixelization
    {
        BeginRenderPass(s_RendererStorage->PixelizationFramebuffer, glm::vec4(0.23f, 0.32f, 0.0f, 1.0f));

        SubmitFullscreenQuad(s_RendererStorage->PixelizationPipeline);

        EndRenderPass(s_RendererStorage->PixelizationFramebuffer);
    }

    s_Renderer->EndQuery();
}

void Renderer::BeginScene(const Camera& camera)
{
    s_RendererStorage->UBGlobalCamera.Projection = camera.GetProjectionMatrix();
    s_RendererStorage->UBGlobalCamera.View       = camera.GetViewMatrix();
    s_RendererStorage->UBGlobalCamera.Position   = camera.GetPosition();

    s_RendererStorage->CameraUniformBuffer->Update(&s_RendererStorage->UBGlobalCamera, sizeof(UBCamera));
    Renderer2D::GetStorageData().CameraProjectionMatrix = camera.GetViewProjectionMatrix();
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

    // TODO: Culling-Pass

    // ShadowMap-Pass
    {
        s_RendererStorage->ShadowMapFramebuffer->SetDepthStencilClearColor(GetSettings().RenderShadows ? 1.0f : 0.0f, 0);
        BeginRenderPass(s_RendererStorage->ShadowMapFramebuffer, glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));

        if (Renderer::GetSettings().RenderShadows)
        {
            constexpr float cameraWidth     = 24.0f;
            constexpr float zNear           = 0.0000001f;
            constexpr float zFar            = 512.0f;
            const glm::mat4 lightProjection = glm::ortho(-cameraWidth, cameraWidth, -cameraWidth, cameraWidth, zNear, zFar);

            const glm::mat4 lightView = glm::lookAt(glm::vec3(s_RendererStorage->UBGlobalLighting.DirLights[0].Direction), glm::vec3(0.0f),
                                                    glm::vec3(0.0f, 1.0f, 0.0f));

            s_RendererStorage->MeshShadowsBuffer.LightSpaceMatrix = lightProjection * lightView;

            LightPushConstants pushConstants   = {};
            pushConstants.LightSpaceProjection = s_RendererStorage->MeshShadowsBuffer.LightSpaceMatrix;

            for (auto& geometry : s_RendererStorage->SortedGeometry)
            {
                pushConstants.Model = geometry.Transform;
                SubmitMesh(s_RendererStorage->ShadowMapPipeline, geometry.VertexBuffer, geometry.IndexBuffer, nullptr, &pushConstants);
            }
        }

        EndRenderPass(s_RendererStorage->ShadowMapFramebuffer);
    }

    // Actual Geometry-Pass
    //{
    //    BeginRenderPass(s_RendererStorage->GeometryFramebuffer, glm::vec4(0.5f, 0.0f, 0.0f, 1.0f));

    //    s_RendererStorage->LightingUniformBuffer->Update(&s_RendererStorage->UBGlobalLighting, sizeof(UBBlinnPhong));

    //    for (auto& geometry : s_RendererStorage->SortedGeometry)
    //    {
    //        MeshPushConstants pushConstants = {};
    //        pushConstants.TransformMatrix   = geometry.Transform;

    //        SubmitMesh(s_RendererStorage->GeometryPipeline, geometry.VertexBuffer, geometry.IndexBuffer, geometry.Material,
    //        &pushConstants);
    //    }

    //    EndRenderPass(s_RendererStorage->GeometryFramebuffer);
    //}

    // PBR-Forward
    {
        BeginRenderPass(s_RendererStorage->PBRFramebuffer, glm::vec4(0.5f, 0.0f, 0.0f, 1.0f));

        for (auto& geometry : s_RendererStorage->SortedGeometry)
        {
            MeshPushConstants pushConstants = {};
            pushConstants.TransformMatrix   = geometry.Transform;

            SubmitMesh(s_RendererStorage->PBRPipeline, geometry.VertexBuffer, geometry.IndexBuffer, geometry.Material, &pushConstants);
        }

        EndRenderPass(s_RendererStorage->PBRFramebuffer);
    }
}

void Renderer::AddPointLight(const glm::vec3& position, const glm::vec3& color, const glm::vec3& AmbientSpecularShininess,
                             const glm::vec3& CLQ)
{
    if (s_RendererStorage->CurrentPointLightIndex >= s_MAX_POINT_LIGHTS) return;

    s_RendererStorage->UBGlobalLighting.PointLights[s_RendererStorage->CurrentPointLightIndex].Position = glm::vec4(position, 0.0f);
    s_RendererStorage->UBGlobalLighting.PointLights[s_RendererStorage->CurrentPointLightIndex].Color    = glm::vec4(color, 0.0f);
    s_RendererStorage->UBGlobalLighting.PointLights[s_RendererStorage->CurrentPointLightIndex].AmbientSpecularShininessCastShadows =
        glm::vec4(AmbientSpecularShininess, 0.0f);

    const bool bIsActive = color.x > 0.0f || color.y > 0.0f || color.z > 0.0f;
    s_RendererStorage->UBGlobalLighting.PointLights[s_RendererStorage->CurrentPointLightIndex].CLQActive = glm::vec4(CLQ, bIsActive);
    ++s_RendererStorage->CurrentPointLightIndex;
}

void Renderer::AddDirectionalLight(const glm::vec3& color, const glm::vec3& direction, const glm::vec4& AmbientSpecularShininessCastShadows)
{
    s_RendererStorage->UBGlobalLighting.Gamma = s_RendererSettings.Gamma;

    if (s_RendererStorage->CurrentDirLightIndex >= s_MAX_DIR_LIGHTS) return;

    s_RendererStorage->UBGlobalLighting.DirLights[s_RendererStorage->CurrentDirLightIndex].Color     = glm::vec4(color, 0.0f);
    s_RendererStorage->UBGlobalLighting.DirLights[s_RendererStorage->CurrentDirLightIndex].Direction = glm::vec4(direction, 0.0f);
    s_RendererStorage->UBGlobalLighting.DirLights[s_RendererStorage->CurrentDirLightIndex].AmbientSpecularShininessCastShadows =
        AmbientSpecularShininessCastShadows;
    ++s_RendererStorage->CurrentDirLightIndex;
}

void Renderer::AddSpotLight(const glm::vec3& position, const glm::vec3& direction, const glm::vec3& color,
                            const glm::vec3& ambientSpecularShininess, const int32_t active, const float cutOff)
{
    if (s_RendererStorage->CurrentSpotLightIndex >= s_MAX_SPOT_LIGHTS) return;

    s_RendererStorage->UBGlobalLighting.SpotLights[s_RendererStorage->CurrentSpotLightIndex].Position  = glm::vec4(position, 0.0f);
    s_RendererStorage->UBGlobalLighting.SpotLights[s_RendererStorage->CurrentSpotLightIndex].Color     = glm::vec4(color, 0.0f);
    s_RendererStorage->UBGlobalLighting.SpotLights[s_RendererStorage->CurrentSpotLightIndex].Direction = glm::vec4(direction, 0.0f);
    s_RendererStorage->UBGlobalLighting.SpotLights[s_RendererStorage->CurrentSpotLightIndex].CutOff    = cutOff;
    s_RendererStorage->UBGlobalLighting.SpotLights[s_RendererStorage->CurrentSpotLightIndex].Ambient   = ambientSpecularShininess.x;
    s_RendererStorage->UBGlobalLighting.SpotLights[s_RendererStorage->CurrentSpotLightIndex].Specular  = ambientSpecularShininess.y;
    s_RendererStorage->UBGlobalLighting.SpotLights[s_RendererStorage->CurrentSpotLightIndex].Shininess = ambientSpecularShininess.z;
    s_RendererStorage->UBGlobalLighting.SpotLights[s_RendererStorage->CurrentSpotLightIndex].Active    = active;

    ++s_RendererStorage->CurrentSpotLightIndex;
}

void Renderer::SubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& transform)
{
    for (uint32_t i = 0; i < mesh->GetSubmeshCount(); ++i)
        s_RendererStorage->SortedGeometry.emplace_back(mesh->GetMaterial(i), mesh->GetVertexBuffers()[i], mesh->GetIndexBuffers()[i],
                                                       transform);
}

std::vector<RendererOutput> Renderer::GetRendererOutput()
{
    std::vector<RendererOutput> rendererOutput;

    const uint32_t currentFrame = GraphicsContext::Get().GetCurrentFrameIndex();

    rendererOutput.emplace_back(s_RendererStorage->ShadowMapFramebuffer->GetAttachments()[0].Attachments[currentFrame], "ShadowMap");
    rendererOutput.emplace_back(s_RendererStorage->GeometryFramebuffer->GetAttachments()[0].Attachments[currentFrame], "Position");
    rendererOutput.emplace_back(s_RendererStorage->GeometryFramebuffer->GetAttachments()[1].Attachments[currentFrame], "Normal");
    rendererOutput.emplace_back(s_RendererStorage->GeometryFramebuffer->GetAttachments()[2].Attachments[currentFrame], "Albedo");
    rendererOutput.emplace_back(s_RendererStorage->GeometryFramebuffer->GetAttachments()[3].Attachments[currentFrame], "Depth");
    rendererOutput.emplace_back(s_RendererStorage->SSAOFramebuffer->GetAttachments()[0].Attachments[currentFrame], "SSAO");
    rendererOutput.emplace_back(s_RendererStorage->SSAOBlurFramebuffer->GetAttachments()[0].Attachments[currentFrame], "SSAO-Blur");
    rendererOutput.emplace_back(s_RendererStorage->ChromaticAberrationFramebuffer->GetAttachments()[0].Attachments[currentFrame],
                                "ChromaticAberration");
    rendererOutput.emplace_back(s_RendererStorage->PixelizationFramebuffer->GetAttachments()[0].Attachments[currentFrame], "Pixelization");

    return rendererOutput;
}

}  // namespace Gauntlet