#pragma once

#include "RendererAPI.h"

#include "Gauntlet/Core/Math.h"

#include "Buffer.h"
#include "CoreRendererStructs.h"
#include "GraphicsContext.h"

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
class CommandBuffer;
class ParticleSystem;

struct RendererOutput
{
    Ref<Image> Attachment;
    std::string Name;
};

class Renderer : private Uncopyable, private Unmovable
{
  public:
    static void Init();
    static void Shutdown();

    static void Begin();
    static void Flush();

    static void BeginScene(const Camera& camera);
    static void EndScene();

    FORCEINLINE static void SubmitParticleSystem(const Ref<CommandBuffer>& commandBuffer, Ref<Pipeline>& pipeline, Ref<StorageBuffer>& ssbo,
                                                 uint32_t particleCount, void* pushConstants = nullptr)
    {
        s_Renderer->SubmitParticleSystemImpl(commandBuffer, pipeline, ssbo, particleCount, pushConstants);
    }

    FORCEINLINE static void BeginRenderPass(const Ref<CommandBuffer>& commandBuffer, const Ref<Framebuffer>& framebuffer,
                                            const glm::vec4& debugLabelColor = glm::vec4(1.0f))
    {
        s_Renderer->BeginRenderPassImpl(commandBuffer, framebuffer, debugLabelColor);
    }
    FORCEINLINE static void EndRenderPass(const Ref<CommandBuffer>& commandBuffer, const Ref<Framebuffer>& framebuffer)
    {
        s_Renderer->EndRenderPassImpl(commandBuffer, framebuffer);
    }

    FORCEINLINE static void SubmitMesh(Ref<Pipeline>& pipeline, Ref<VertexBuffer>& vertexBuffer, Ref<IndexBuffer>& indexBuffer,
                                       Ref<Material> material, void* pushConstants = nullptr)
    {
        s_Renderer->SubmitMeshImpl(pipeline, vertexBuffer, indexBuffer, material, pushConstants);
    }

    FORCEINLINE static void Dispatch(Ref<CommandBuffer>& commandBuffer, Ref<Pipeline>& pipeline, void* pushConstants = nullptr,
                                     const uint32_t groupCountX = 1, const uint32_t groupCountY = 1, const uint32_t groupCountZ = 1)
    {
        s_Renderer->DispatchImpl(commandBuffer, pipeline, pushConstants, groupCountX, groupCountY, groupCountZ);
    }

    FORCEINLINE static void SubmitFullscreenQuad(Ref<Pipeline>& pipeline, void* pushConstants = nullptr)
    {
        s_Renderer->SubmitFullscreenQuadImpl(pipeline, pushConstants);
    }

    FORCEINLINE static void DrawQuad(Ref<Pipeline>& pipeline, Ref<VertexBuffer>& vertexBuffer, Ref<IndexBuffer>& indexBuffer,
                                     const uint32_t indicesCount, void* pushConstants = nullptr)
    {
        s_Renderer->DrawQuadImpl(pipeline, vertexBuffer, indexBuffer, indicesCount, pushConstants);
    }

    static void SubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& transform = glm::mat4(1.0f));
    static void AddPointLight(const glm::vec3& position, const glm::vec3& color, const float intensity, int32_t active);

    static void AddDirectionalLight(const glm::vec3& color, const glm::vec3& direction, int32_t castShadows, float intensity);

    static void AddSpotLight(const glm::vec3& position, const glm::vec3& direction, const glm::vec3& color, const float intensity,
                             const int32_t active, const float cutOff, const float outerCutOff);

    FORCEINLINE static void ResizeFramebuffers(uint32_t width, uint32_t height)
    {
        s_RendererStorage->bFramebuffersNeedResize = true;
        s_RendererStorage->NewFramebufferSize      = {width, height};
    }

    FORCEINLINE static auto& GetStats() { return s_RendererStats; }
    FORCEINLINE static auto& GetSettings() { return s_RendererSettings; }
    static const std::vector<std::string> GetPipelineStatistics();

    static const Ref<Image>& GetFinalImage();
    FORCEINLINE static std::mutex& GetResourceAccessMutex() { return s_ResourceAccessMutex; }

    static std::vector<RendererOutput> GetRendererOutput();
    FORCEINLINE static const auto& GetStorageData() { return *s_RendererStorage; }

  private:
    static Renderer* s_Renderer;
    static std::mutex s_ResourceAccessMutex;

    struct GeometryData
    {
        Ref<Gauntlet::Material> Material;
        Ref<Gauntlet::VertexBuffer> VertexBuffer;
        Ref<Gauntlet::IndexBuffer> IndexBuffer;
        glm::mat4 Transform;
    };

    static void CollectPassStatistics();

  protected:
    struct RendererSettings
    {
        float Gamma                  = 2.2f;
        float Exposure               = 1.0f;
        bool ShowWireframes          = false;
        bool VSync                   = false;
        bool ChromaticAberrationView = false;
        uint32_t ParticleCount       = 500;

        struct
        {
            bool RenderShadows           = true;
            bool SoftShadows             = false;
            uint16_t CurrentShadowPreset = 0;
            // "Low" - "Resolution", since shadow images can have OxO resolution.
            const std::vector<std::pair<std::string, uint16_t>> ShadowPresets = {
                {"Low", 1024},  //
                {"Medium", 2048},
                {"High", 4096},
            };
        } Shadows;

        struct
        {
            bool EnableSSAO   = true;
            bool BlurSSAO     = true;
            float Radius      = 0.5f;
            float Bias        = 0.025f;
            int32_t Magnitude = 1;
        } AO;
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

        std::string RenderingDevice                   = "None";
        static constexpr size_t s_MaxUploadHeapSizeMB = 4 * 1024 * 1024;
        size_t UploadHeapCapacity                     = 0;

        std::vector<size_t> PipelineStatisticsResults;
        std::vector<std::string> PassStatistsics;
    } static s_RendererStats;

    struct RendererStorage
    {
        // Viewports
        bool bFramebuffersNeedResize  = {false};
        glm::uvec2 NewFramebufferSize = {1280, 720};
        std::vector<Ref<CommandBuffer>> RenderCommandBuffer;  // per-frame

        Ref<ParticleSystem> GPUParticleSystem;

        // Defaults
        BufferLayout StaticMeshVertexBufferLayout;  // can remove?
        BufferLayout AnimatedVertexBufferLayout;    // can remove?
        Ref<Texture2D> WhiteTexture = nullptr;

        // Clear-Pass
        Ref<Framebuffer> SetupFramebuffer = nullptr;

        // GBuffer (Deferred rendering)
        Ref<Framebuffer> GeometryFramebuffer = nullptr;
        Ref<Pipeline> GeometryPipeline       = nullptr;

        // PBR-Forward
        Ref<Framebuffer> PBRFramebuffer = nullptr;
        Ref<Pipeline> PBRPipeline       = nullptr;

        // ShadowMapping
        Ref<Framebuffer> ShadowMapFramebuffer = nullptr;
        Ref<Pipeline> ShadowMapPipeline       = nullptr;

        // Shadows UB
        std::array<Ref<UniformBuffer>, FRAMES_IN_FLIGHT> ShadowsUniformBuffer;
        UBShadows MeshShadowsBuffer;

        // SSAO
        Ref<Framebuffer> SSAOFramebuffer = nullptr;
        Ref<Pipeline> SSAOPipeline       = nullptr;

        // SSAO UB
        std::array<Ref<UniformBuffer>, FRAMES_IN_FLIGHT> SSAOUniformBuffer;
        UBSSAO SSAODataBuffer;

        // SSAO-Blur
        Ref<Framebuffer> SSAOBlurFramebuffer = nullptr;
        Ref<Pipeline> SSAOBlurPipeline       = nullptr;
        Ref<Texture2D> SSAONoiseTexture      = nullptr;

        // Animation
        Ref<Pipeline> AnimationPipeline = nullptr;

        // Final Lighting
        Ref<Framebuffer> LightingFramebuffer = nullptr;
        Ref<Pipeline> LightingPipeline       = nullptr;

        // Chromatic Aberration
        Ref<Framebuffer> ChromaticAberrationFramebuffer = nullptr;
        Ref<Pipeline> ChromaticAberrationPipeline       = nullptr;

        // Camera UB
        std::array<Ref<UniformBuffer>, FRAMES_IN_FLIGHT> CameraUniformBuffer;
        UBCamera UBGlobalCamera;

        // Global lighting UB
        std::array<Ref<UniformBuffer>, FRAMES_IN_FLIGHT> LightingUniformBuffer;
        UBLighting UBGlobalLighting;
        uint32_t CurrentPointLightIndex = 0;
        uint32_t CurrentDirLightIndex   = 0;
        uint32_t CurrentSpotLightIndex  = 0;

        // Misc
        std::vector<GeometryData> SortedGeometry;
        Ref<StagingBuffer> UploadHeap = nullptr;
    } static* s_RendererStorage;

  protected:
    Renderer()          = default;
    virtual ~Renderer() = default;

    virtual void BeginImpl() = 0;

    virtual void SubmitParticleSystemImpl(const Ref<CommandBuffer>& commandBuffer, Ref<Pipeline>& pipeline, Ref<StorageBuffer>& ssbo,
                                          uint32_t particleCount, void* pushConstants = nullptr)                              = 0;
    virtual void DispatchImpl(Ref<CommandBuffer>& commandBuffer, Ref<Pipeline>& pipeline, void* pushConstants = nullptr,
                              const uint32_t groupCountX = 1, const uint32_t groupCountY = 1, const uint32_t groupCountZ = 1) = 0;

    virtual void BeginRenderPassImpl(const Ref<CommandBuffer>& commandBuffer, const Ref<Framebuffer>& framebuffer,
                                     const glm::vec4& debugLabelColor = glm::vec4(1.0f))                         = 0;
    virtual void EndRenderPassImpl(const Ref<CommandBuffer>& commandBuffer, const Ref<Framebuffer>& framebuffer) = 0;

    virtual void SubmitFullscreenQuadImpl(Ref<Pipeline>& pipeline, void* pushConstants = nullptr) = 0;
    virtual void SubmitMeshImpl(Ref<Pipeline>& pipeline, Ref<VertexBuffer>& vertexBuffer, Ref<IndexBuffer>& indexBuffer,
                                Ref<Material>& material, void* pushConstants = nullptr)           = 0;

    virtual void DrawQuadImpl(Ref<Pipeline>& pipeline, Ref<VertexBuffer>& vertexBuffer, Ref<IndexBuffer>& indexBuffer,
                              const uint32_t indicesCount, void* pushConstants = nullptr) = 0;
};

}  // namespace Gauntlet