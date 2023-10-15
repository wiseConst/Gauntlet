#include "GauntletPCH.h"
#include "Renderer.h"
#include "Shader.h"
#include "Texture.h"
#include "Pipeline.h"
#include "Framebuffer.h"
#include "Mesh.h"
#include "Camera/Camera.h"
#include "GraphicsContext.h"

#include "Gauntlet/Core/Random.h"
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
        attachment.Filter                             = ETextureFilter::NEAREST;
        attachment.Wrap                               = ETextureWrap::CLAMP;

        // Position
        attachment.Format = EImageFormat::RGBA16F;
        geometryFramebufferSpec.Attachments.push_back(attachment);

        attachment.Wrap = ETextureWrap::REPEAT;

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

    // Clear-Pass
    {
        FramebufferSpecification setupFramebufferSpec = {};
        setupFramebufferSpec.ExistingAttachments      = s_RendererStorage->GeometryFramebuffer->GetAttachments();
        setupFramebufferSpec.Name                     = "Setup";

        s_RendererStorage->SetupFramebuffer = Framebuffer::Create(setupFramebufferSpec);
    }

    // ShadowMap
    {
        FramebufferSpecification shadowMapFramebufferSpec = {};

        FramebufferAttachmentSpecification shadowMapAttachment = {};
        shadowMapAttachment.ClearColor                         = glm::vec4(1.0f, glm::vec3(0.0f));
        shadowMapAttachment.Format                             = EImageFormat::DEPTH32F;
        shadowMapAttachment.LoadOp                             = ELoadOp::CLEAR;
        shadowMapAttachment.StoreOp                            = EStoreOp::STORE;
        shadowMapAttachment.Filter                             = ETextureFilter::LINEAR;
        shadowMapAttachment.Wrap                               = ETextureWrap::CLAMP;

        shadowMapFramebufferSpec.Attachments = {shadowMapAttachment};
        shadowMapFramebufferSpec.Name        = "ShadowMap";
        shadowMapFramebufferSpec.Width       = 1024;
        shadowMapFramebufferSpec.Height      = 1024;

        s_RendererStorage->ShadowMapFramebuffer = Framebuffer::Create(shadowMapFramebufferSpec);

        PipelineSpecification shadowmapPipelineSpec = {};
        shadowmapPipelineSpec.Name                  = "ShadowMap";
        shadowmapPipelineSpec.PolygonMode           = EPolygonMode::POLYGON_MODE_FILL;
        shadowmapPipelineSpec.FrontFace             = EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE;
        // shadowmapPipelineSpec.CullMode              = ECullMode::CULL_MODE_FRONT;  // ???
        shadowmapPipelineSpec.bDepthWrite       = true;
        shadowmapPipelineSpec.bDepthTest        = true;
        shadowmapPipelineSpec.DepthCompareOp    = ECompareOp::COMPARE_OP_LESS;
        shadowmapPipelineSpec.TargetFramebuffer = s_RendererStorage->ShadowMapFramebuffer;
        shadowmapPipelineSpec.Layout            = s_RendererStorage->MeshVertexBufferLayout;
        shadowmapPipelineSpec.Shader            = ShaderLibrary::Load("Resources/Cached/Shaders/Depth");

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
        ssaoFramebufferAttachmentSpec.Filter                             = ETextureFilter::NEAREST;
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
        attachment.Format                             = EImageFormat::RGBA16F;
        attachment.LoadOp                             = ELoadOp::CLEAR;
        attachment.StoreOp                            = EStoreOp::STORE;
        lightingFramebufferSpec.Attachments.push_back(attachment);

        lightingFramebufferSpec.Name           = "FinalLightingPass";
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

    // SSAO
    {
        for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            s_RendererStorage->SSAOPipeline->GetSpecification().Shader->Set(
                "u_PositionMap", s_RendererStorage->GeometryFramebuffer->GetAttachments()[0].Attachments[frame]);

            s_RendererStorage->SSAOPipeline->GetSpecification().Shader->Set(
                "u_NormalMap", s_RendererStorage->GeometryFramebuffer->GetAttachments()[1].Attachments[frame]);

            s_RendererStorage->SSAOPipeline->GetSpecification().Shader->Set("u_TexNoiseMap", s_RendererStorage->SSAONoiseTexture);

            s_RendererStorage->SSAOPipeline->GetSpecification().Shader->Set("u_UBSSAO", s_RendererStorage->SSAOUniformBuffer, 0);
        }
    }

    // Lighting
    {
        for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
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
    }

    s_Renderer->PostInit();
}

void Renderer::Shutdown()
{
    GraphicsContext::Get().WaitDeviceOnFinish();

    ShaderLibrary::Shutdown();

    s_RendererStorage->GeometryFramebuffer->Destroy();
    s_RendererStorage->GeometryPipeline->Destroy();

    s_RendererStorage->SetupFramebuffer->Destroy();

    s_RendererStorage->ForwardPassPipeline->Destroy();

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

    s_RendererStorage->CameraUniformBuffer->Destroy();

    s_RendererStorage->WhiteTexture->Destroy();
    delete s_RendererStorage;
    s_RendererStorage = nullptr;

    delete s_Renderer;
    s_Renderer = nullptr;
}

const Ref<Image>& Renderer::GetFinalImage()
{
    return s_RendererStorage->LightingFramebuffer->GetAttachments()[0].Attachments[GraphicsContext::Get().GetCurrentFrameIndex()];
}

void Renderer::Begin()
{
    s_Renderer->BeginImpl();
    Renderer::GetStats().DrawCalls = 0;

    if (s_RendererStorage->bFramebuffersNeedResize)
    {
        // ShadowMapFramebuffer has fixed size independent of screen size
        s_RendererStorage->GeometryFramebuffer->Resize(s_RendererStorage->NewFramebufferSize.x, s_RendererStorage->NewFramebufferSize.y);
        s_RendererStorage->SetupFramebuffer->Resize(s_RendererStorage->NewFramebufferSize.x, s_RendererStorage->NewFramebufferSize.y);
        s_RendererStorage->SSAOFramebuffer->Resize(s_RendererStorage->NewFramebufferSize.x, s_RendererStorage->NewFramebufferSize.y);
        s_RendererStorage->SSAOBlurFramebuffer->Resize(s_RendererStorage->NewFramebufferSize.x, s_RendererStorage->NewFramebufferSize.y);
        s_RendererStorage->LightingFramebuffer->Resize(s_RendererStorage->NewFramebufferSize.x, s_RendererStorage->NewFramebufferSize.y);

        /*for (auto& geometry : s_Data.SortedGeometry)
        {
            geometry.Material->Invalidate();
        }*/

        // Updating LightingSet
        for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
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
        }

        // Updating SSAO Set
        for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            s_RendererStorage->SSAOPipeline->GetSpecification().Shader->Set(
                "u_PositionMap", s_RendererStorage->GeometryFramebuffer->GetAttachments()[0].Attachments[frame]);

            s_RendererStorage->SSAOPipeline->GetSpecification().Shader->Set(
                "u_NormalMap", s_RendererStorage->GeometryFramebuffer->GetAttachments()[1].Attachments[frame]);

            s_RendererStorage->SSAOPipeline->GetSpecification().Shader->Set("u_TexNoiseMap", s_RendererStorage->SSAONoiseTexture);

            s_RendererStorage->SSAOPipeline->GetSpecification().Shader->Set("u_UBSSAO", s_RendererStorage->SSAOUniformBuffer, 0);
        }

        // Updating SSAO-Blur Set
        for (uint32_t frame = 0; frame < FRAMES_IN_FLIGHT; ++frame)
        {
            s_RendererStorage->SSAOBlurPipeline->GetSpecification().Shader->Set(
                "u_SSAOMap", s_RendererStorage->SSAOFramebuffer->GetAttachments()[0].Attachments[frame]);
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

    // Clear geometry buffer contents
    BeginRenderPass(s_RendererStorage->SetupFramebuffer, glm::vec4(0.3f, 0.56f, 0.841f, 1.0f));
    EndRenderPass(s_RendererStorage->SetupFramebuffer);

    {
        s_RendererStorage->UBGlobalLighting.Gamma    = s_RendererSettings.Gamma;
        s_RendererStorage->UBGlobalLighting.DirLight = DirectionalLight();

        for (uint32_t i = 0; i < s_RendererStorage->CurrentPointLightIndex; ++i)
            s_RendererStorage->UBGlobalLighting.PointLights[i] = PointLight();
    }
    s_RendererStorage->CurrentPointLightIndex = 0;

    s_RendererStorage->SortedGeometry.clear();
}

void Renderer::Flush()
{
    s_Renderer->FlushImpl();

    // SSAO-Pass
    if (!s_RendererStorage->SortedGeometry.empty() && Renderer::GetSettings().EnableSSAO)
    {
        static constexpr auto noiseFactor                         = 4.0f;
        s_RendererStorage->SSAODataBuffer.ViewportSizeNoiseFactor = glm::vec4(s_RendererStorage->NewFramebufferSize, noiseFactor, 0.0f);
        s_RendererStorage->SSAODataBuffer.CameraProjection        = s_RendererStorage->UBGlobalCamera.Projection;

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
}

void Renderer::BeginScene(const Camera& camera)
{
    s_RendererStorage->UBGlobalCamera.Projection = camera.GetProjectionMatrix();
    s_RendererStorage->UBGlobalCamera.View       = camera.GetViewMatrix();
    s_RendererStorage->UBGlobalCamera.Position   = camera.GetPosition();

    s_RendererStorage->CameraUniformBuffer->Update(&s_RendererStorage->UBGlobalCamera, sizeof(UBCamera));
    s_Renderer->BeginSceneImpl(camera);
}

void Renderer::EndScene()
{
    // Proper depth testing?
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

    s_Renderer->EndSceneImpl();

    // ShadowMap-Pass
    {
        s_RendererStorage->ShadowMapFramebuffer->SetDepthStencilClearColor(GetSettings().RenderShadows ? 1.0f : 0.0f, 0);
        BeginRenderPass(s_RendererStorage->ShadowMapFramebuffer, glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));

        if (Renderer::GetSettings().RenderShadows)
        {
            constexpr float cameraWidth     = 24.0f;
            constexpr float zNear           = 0.000001f;
            constexpr float zFar            = 512.0f;
            const glm::mat4 lightProjection = glm::ortho(-cameraWidth, cameraWidth, -cameraWidth, cameraWidth, zNear, zFar);

            const glm::mat4 lightView = glm::lookAt(glm::vec3(s_RendererStorage->UBGlobalLighting.DirLight.Direction), glm::vec3(0.0f),
                                                    glm::vec3(0.0f, 1.0f, 0.0f));

            s_RendererStorage->MeshShadowsBuffer.LightSpaceMatrix = lightProjection * lightView;

            LightPushConstants pushConstants   = {};
            pushConstants.LightSpaceProjection = s_RendererStorage->MeshShadowsBuffer.LightSpaceMatrix;
            pushConstants.Model                = glm::mat4(1.0f);

            for (auto& geometry : s_RendererStorage->SortedGeometry)
            {
                RenderGeometry(s_RendererStorage->ShadowMapPipeline, geometry, false, &pushConstants);
            }
        }

        EndRenderPass(s_RendererStorage->ShadowMapFramebuffer);
    }

    // Actual Geometry-Pass
    {
        BeginRenderPass(s_RendererStorage->GeometryFramebuffer, glm::vec4(0.5f, 0.0f, 0.0f, 1.0f));

        s_RendererStorage->LightingUniformBuffer->Update(&s_RendererStorage->UBGlobalLighting, sizeof(UBBlinnPhong));
        for (auto& geometry : s_RendererStorage->SortedGeometry)
        {
            MeshPushConstants pushConstants = {};
            pushConstants.TransformMatrix   = geometry.Transform;
            RenderGeometry(s_RendererStorage->GeometryPipeline, geometry, true, &pushConstants);
        }

        EndRenderPass(s_RendererStorage->GeometryFramebuffer);
    }
}

void Renderer::AddPointLight(const glm::vec3& position, const glm::vec3& color, const glm::vec3& AmbientSpecularShininess,
                             const glm::vec3& CLQ)
{
    if (s_RendererStorage->CurrentPointLightIndex >= s_MAX_POINT_LIGHTS) return;

    s_RendererStorage->UBGlobalLighting.PointLights[s_RendererStorage->CurrentPointLightIndex].Position = glm::vec4(position, 0.0f);
    s_RendererStorage->UBGlobalLighting.PointLights[s_RendererStorage->CurrentPointLightIndex].Color    = glm::vec4(color, 0.0f);
    s_RendererStorage->UBGlobalLighting.PointLights[s_RendererStorage->CurrentPointLightIndex].AmbientSpecularShininess =
        glm::vec4(AmbientSpecularShininess, 0.0f);

    const bool bIsActive = color.x > 0.0f || color.y > 0.0f || color.z > 0.0f;
    s_RendererStorage->UBGlobalLighting.PointLights[s_RendererStorage->CurrentPointLightIndex].CLQActive = glm::vec4(CLQ, bIsActive);

    ++s_RendererStorage->CurrentPointLightIndex;
}

void Renderer::AddDirectionalLight(const glm::vec3& color, const glm::vec3& direction, const glm::vec4& AmbientSpecularShininessCastShadows)
{
    s_RendererStorage->UBGlobalLighting.Gamma = s_RendererSettings.Gamma;

    s_RendererStorage->UBGlobalLighting.DirLight.Color                    = glm::vec4(color, 0.0f);
    s_RendererStorage->UBGlobalLighting.DirLight.Direction                = glm::vec4(direction, 0.0f);
    s_RendererStorage->UBGlobalLighting.DirLight.AmbientSpecularShininess = AmbientSpecularShininessCastShadows;
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

    return rendererOutput;
}

}  // namespace Gauntlet