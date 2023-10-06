#pragma once

#include "RendererAPI.h"

#include <glm/glm.hpp>
#include "Buffer.h"

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

struct RendererOutput
{
    Ref<Image> Attachment;
    std::string Name;
};

class Renderer : private Uncopyable, private Unmovable
{
  public:
    Renderer()  = default;
    ~Renderer() = default;

    static void Init();
    static void Shutdown();

    static void Begin();
    static void Flush();

    FORCEINLINE static void BeginScene(const Camera& camera) { s_Renderer->BeginSceneImpl(camera); }
    FORCEINLINE static void EndScene() { s_Renderer->EndSceneImpl(); }
    FORCEINLINE static void SubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& transform = glm::mat4(1.0f))
    {
        s_Renderer->SubmitMeshImpl(mesh, transform);
    }

    FORCEINLINE static void AddPointLight(const glm::vec3& position, const glm::vec3& color, const glm::vec3& AmbientSpecularShininess,
                                          const glm::vec3& CLQ)
    {
        s_Renderer->AddPointLightImpl(position, color, AmbientSpecularShininess, CLQ);
    }

    FORCEINLINE static void AddDirectionalLight(const glm::vec3& color, const glm::vec3& direction,
                                                const glm::vec4& AmbientSpecularShininessCastShadows)
    {
        s_Renderer->AddDirectionalLightImpl(color, direction, AmbientSpecularShininessCastShadows);
    }

    FORCEINLINE static void ResizeFramebuffers(uint32_t width, uint32_t height) { s_Renderer->ResizeFramebuffersImpl(width, height); };

    FORCEINLINE static auto& GetStats() { return s_RendererStats; }
    FORCEINLINE static auto& GetSettings() { return s_RendererSettings; }

    FORCEINLINE static const Ref<Image>& GetFinalImage() { return s_Renderer->GetFinalImageImpl(); }
    FORCEINLINE static std::mutex& GetResourceAccessMutex() { return s_ResourceAccessMutex; }

    FORCEINLINE static std::vector<RendererOutput> GetRendererOutput() { return s_Renderer->GetRendererOutputImpl(); }
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
        bool BlurSSAO     = true;
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
        BufferLayout MeshVertexBufferLayout;

        // Defaults
        Ref<Texture2D> WhiteTexture = nullptr;
    } static* s_RendererStorage;

  protected:
    virtual void Create()  = 0;
    virtual void Destroy() = 0;

    virtual void AddPointLightImpl(const glm::vec3& position, const glm::vec3& color, const glm::vec3& AmbientSpecularShininess,
                                   const glm::vec3& CLQ)                                       = 0;
    virtual void AddDirectionalLightImpl(const glm::vec3& color, const glm::vec3& direction,
                                         const glm::vec4& AmbientSpecularShininessCastShadows) = 0;

    virtual void BeginSceneImpl(const Camera& camera) = 0;
    virtual void EndSceneImpl()                       = 0;

    virtual void SubmitMeshImpl(const Ref<Mesh>& mesh, const glm::mat4& transform) = 0;
    virtual void ResizeFramebuffersImpl(uint32_t width, uint32_t height)           = 0;

    virtual void BeginImpl() = 0;
    virtual void FlushImpl() = 0;

    virtual const Ref<Image>& GetFinalImageImpl()               = 0;
    virtual std::vector<RendererOutput> GetRendererOutputImpl() = 0;
};

}  // namespace Gauntlet