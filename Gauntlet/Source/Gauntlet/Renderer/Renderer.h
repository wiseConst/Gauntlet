#pragma once

#include "RendererAPI.h"

#include "Gauntlet/Core/Math.h"

#include "Buffer.h"
#include "CoreRendererStructs.h"

namespace Gauntlet
{

#define GRAPHICS_GUARD_LOCK std::unique_lock<std::mutex> Lock(Renderer::GetResourceAccessMutex())

class Mesh;
class Camera;
class Image;
class Texture2D;

class Material;
class VertexBuffer;
class IndexBuffer;
class Framebuffer;
class Pipeline;

struct RendererOutput
{
    Ref<Image> Attachment;
    std::string Name;
};

class Renderer : private Uncopyable, private Unmovable
{
  protected:
    struct GeometryData
    {
        Ref<Gauntlet::Material> Material;
        Ref<Gauntlet::VertexBuffer> VertexBuffer;
        Ref<Gauntlet::IndexBuffer> IndexBuffer;
        glm::mat4 Transform;
    };

  public:
    Renderer()          = default;
    virtual ~Renderer() = default;

    virtual void PostInit() = 0;

    static void Init();
    static void Shutdown();

    static void Begin();
    static void Flush();

    static void BeginScene(const Camera& camera);
    static void EndScene();

    FORCEINLINE static void BeginRenderPass(const Ref<Framebuffer>& framebuffer, const glm::vec4& debugLabelColor = glm::vec4(1.0f))
    {
        s_Renderer->BeginRenderPassImpl(framebuffer, debugLabelColor);
    }
    FORCEINLINE static void EndRenderPass(const Ref<Framebuffer>& framebuffer) { s_Renderer->EndRenderPassImpl(framebuffer); }

    // TODO: Refactor bWithMaterial(means should it use descriptor set from geometry->Material)
    FORCEINLINE static void RenderGeometry(Ref<Pipeline>& pipeline, const GeometryData& geometry, bool bWithMaterial = true,
                                           void* pushConstants = nullptr)
    {
        s_Renderer->RenderGeometryImpl(pipeline, geometry, bWithMaterial, pushConstants);
    }

    FORCEINLINE static void SubmitFullscreenQuad(Ref<Pipeline>& pipeline, void* pushConstants = nullptr)
    {
        s_Renderer->SubmitFullscreenQuadImpl(pipeline, pushConstants);
    }

    static void SubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& transform = glm::mat4(1.0f));
    static void AddPointLight(const glm::vec3& position, const glm::vec3& color, const glm::vec3& AmbientSpecularShininess,
                              const glm::vec3& CLQ);

    static void AddDirectionalLight(const glm::vec3& color, const glm::vec3& direction,
                                    const glm::vec4& AmbientSpecularShininessCastShadows);

    FORCEINLINE static void ResizeFramebuffers(uint32_t width, uint32_t height)
    {
        s_RendererStorage->bFramebuffersNeedResize = true;
        s_RendererStorage->NewFramebufferSize      = {width, height};
    }

    FORCEINLINE static auto& GetStats() { return s_RendererStats; }
    FORCEINLINE static auto& GetSettings() { return s_RendererSettings; }

    static const Ref<Image>& GetFinalImage();
    FORCEINLINE static std::mutex& GetResourceAccessMutex() { return s_ResourceAccessMutex; }

    static std::vector<RendererOutput> GetRendererOutput();
    FORCEINLINE static const auto& GetStorageData() { return *s_RendererStorage; }

  protected:
    static Renderer* s_Renderer;
    static std::mutex s_ResourceAccessMutex;

    struct RendererSettings
    {
        bool ShowWireframes = false;
        bool VSync          = false;
        bool RenderShadows  = true;
        bool EnableSSAO     = true;
        bool BlurSSAO       = true;
        float Gamma         = 1.1f;
    } static s_RendererSettings;

    struct RendererStats
    {
        std::atomic<size_t> GPUMemoryAllocated = 0;
        std::atomic<size_t> RAMMemoryAllocated = 0;  // Now it only refers to VMA allocations.

        std::atomic<size_t> Allocations      = 0;
        std::atomic<size_t> AllocatedBuffers = 0;
        std::atomic<size_t> AllocatedImages  = 0;

        std::atomic<size_t> DrawCalls = 0;
        std::atomic<size_t> QuadCount = 0;

        std::atomic<uint32_t> AllocatedDescriptorSets = 0;
        uint16_t FPS                                  = 0;

        float CPUWaitTime = 0.0f;
        float GPUWaitTime = 0.0f;
        float PresentTime = 0.0f;
        float FrameTime   = 0.0f;
    } static s_RendererStats;

    struct RendererStorage
    {
        // Viewports
        bool bFramebuffersNeedResize  = {false};
        glm::uvec2 NewFramebufferSize = {1280, 720};

        // Defaults
        BufferLayout MeshVertexBufferLayout;
        Ref<Texture2D> WhiteTexture = nullptr;

        // GBuffer (Deferred rendering)
        Ref<Framebuffer> GeometryFramebuffer = nullptr;
        Ref<Pipeline> GeometryPipeline       = nullptr;

        // Clear-Pass
        Ref<Framebuffer> SetupFramebuffer = nullptr;

        // ShadowMapping
        Ref<Framebuffer> ShadowMapFramebuffer = nullptr;
        Ref<Pipeline> ShadowMapPipeline       = nullptr;

        // Shadows UB
        Ref<UniformBuffer> ShadowsUniformBuffer;
        UBShadows MeshShadowsBuffer;

        // SSAO
        Ref<Framebuffer> SSAOFramebuffer = nullptr;
        Ref<Pipeline> SSAOPipeline       = nullptr;

        // SSAO UB
        Ref<UniformBuffer> SSAOUniformBuffer = nullptr;
        UBSSAO SSAODataBuffer;

        // SSAO-Blur
        Ref<Framebuffer> SSAOBlurFramebuffer = nullptr;
        Ref<Pipeline> SSAOBlurPipeline       = nullptr;
        Ref<Texture2D> SSAONoiseTexture      = nullptr;

        // Forward Rendering
        Ref<Pipeline> ForwardPassPipeline = nullptr;

        // Final Lighting
        Ref<Framebuffer> LightingFramebuffer = nullptr;
        Ref<Pipeline> LightingPipeline       = nullptr;

        // Camera UB
        Ref<UniformBuffer> CameraUniformBuffer = nullptr;
        UBCamera UBGlobalCamera;

        // TODO: Add multiple directional lights
        // Global blinn-phong lighting UB
        Ref<UniformBuffer> LightingUniformBuffer = nullptr;
        UBBlinnPhong UBGlobalLighting;
        uint32_t CurrentPointLightIndex = 0;

        // Skybox on testing
        /* Ref<Skybox> DefaultSkybox          = nullptr;
         Ref<Pipeline> SkyboxPipeline = nullptr;*/

        // Misc
        std::vector<GeometryData> SortedGeometry;
    } static* s_RendererStorage;

  protected:
    virtual void BeginSceneImpl(const Camera& camera) = 0;
    virtual void EndSceneImpl()                       = 0;

    virtual void BeginImpl() = 0;
    virtual void FlushImpl() = 0;

    virtual void BeginRenderPassImpl(const Ref<Framebuffer>& framebuffer, const glm::vec4& debugLabelColor = glm::vec4(1.0f)) = 0;
    virtual void EndRenderPassImpl(const Ref<Framebuffer>& framebuffer)                                                       = 0;

    virtual void SubmitFullscreenQuadImpl(Ref<Pipeline>& pipeline, void* pushConstants = nullptr) = 0;
    virtual void RenderGeometryImpl(Ref<Pipeline>& pipeline, const GeometryData& geometry, bool bWithMaterial = true,
                                    void* pushConstants = nullptr)                                = 0;
};

}  // namespace Gauntlet