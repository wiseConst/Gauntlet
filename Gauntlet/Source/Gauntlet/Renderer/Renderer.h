#pragma once

#include "RendererAPI.h"

#include <glm/glm.hpp>

namespace Gauntlet
{

#define GRAPHICS_GUARD_LOCK std::unique_lock<std::mutex> Lock(Renderer::GetResourceAccessMutex())

class Mesh;
class PerspectiveCamera;
class Image;
class Texture2D;

class Renderer : private Uncopyable, private Unmovable
{
  public:
    Renderer()  = default;
    ~Renderer() = default;

    static void Init();
    static void Shutdown();

    FORCEINLINE static void Begin() { s_Renderer->BeginImpl(); }
    FORCEINLINE static void Flush() { s_Renderer->FlushImpl(); }

    FORCEINLINE static void BeginScene(const PerspectiveCamera& InCamera) { s_Renderer->BeginSceneImpl(InCamera); }
    FORCEINLINE static void SubmitMesh(const Ref<Mesh>& InMesh, const glm::mat4& InTransformMatrix = glm::mat4(1.0f))
    {
        s_Renderer->SubmitMeshImpl(InMesh, InTransformMatrix);
    }

    FORCEINLINE static void AddPointLight(const glm::vec3& Position, const glm::vec3& Color, const glm::vec3& AmbientSpecularShininess,
                                          const glm::vec3& CLQ)
    {
        s_Renderer->AddPointLightImpl(Position, Color, AmbientSpecularShininess, CLQ);
    }

    FORCEINLINE static void AddDirectionalLight(const glm::vec3& Color, const glm::vec3& Direction,
                                                const glm::vec3& AmbientSpecularShininess)
    {
        s_Renderer->AddDirectionalLightImpl(Color, Direction, AmbientSpecularShininess);
    }

    FORCEINLINE static auto& GetStats() { return s_RendererStats; }
    FORCEINLINE static auto& GetSettings() { return s_RendererSettings; }

    FORCEINLINE static const Ref<Image> GetFinalImage() { return s_Renderer->GetFinalImageImpl(); }

    FORCEINLINE static std::mutex& GetResourceAccessMutex() { return s_ResourceAccessMutex; }

  protected:
    static Renderer* s_Renderer;
    static std::mutex s_ResourceAccessMutex;

    struct RendererSettings
    {
        bool ShowWireframes = false;
        bool VSync          = false;
        float Gamma         = 1.1f;
    } static s_RendererSettings;

    struct RendererStats
    {
        size_t GPUMemoryAllocated = 0;

        size_t Allocations          = 0;
        size_t AllocatedBuffers     = 0;
        size_t StagingVertexBuffers = 0;
        size_t AllocatedImages      = 0;

        size_t DrawCalls = 0;
        size_t QuadCount = 0;

        uint32_t AllocatedDescriptorSets = 0;
        uint32_t FPS                     = 0;

        float CPUWaitTime = 0.0f;
        float GPUWaitTime = 0.0f;
    } static s_RendererStats;

  protected:
    virtual void Create()  = 0;
    virtual void Destroy() = 0;

    virtual void AddPointLightImpl(const glm::vec3& Position, const glm::vec3& Color, const glm::vec3& AmbientSpecularShininess,
                                   const glm::vec3& CLQ)                                                                                = 0;
    virtual void AddDirectionalLightImpl(const glm::vec3& Color, const glm::vec3& Direction, const glm::vec3& AmbientSpecularShininess) = 0;

    virtual void BeginSceneImpl(const PerspectiveCamera& InCamera)                           = 0;
    virtual void SubmitMeshImpl(const Ref<Mesh>& InMesh, const glm::mat4& InTransformMatrix) = 0;

    virtual void BeginImpl() = 0;
    virtual void FlushImpl() = 0;

    virtual const Ref<Image> GetFinalImageImpl() = 0;
};

}  // namespace Gauntlet
