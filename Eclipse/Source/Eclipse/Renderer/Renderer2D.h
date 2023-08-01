#pragma once

#include "Eclipse/Core/Core.h"
#include "CoreRendererStructs.h"

#include "Eclipse/Renderer/Camera/Camera.h"

namespace Eclipse
{

class Texture2D;

class Renderer2D : private Uncopyable, private Unmovable
{
  public:
    static void Init();
    static void Shutdown();

    FORCEINLINE static void BeginScene(const Camera& InCamera) { s_Renderer->BeginSceneImpl(InCamera); }

    FORCEINLINE static void Begin() { s_Renderer->BeginImpl(); }
    FORCEINLINE static void Flush() { s_Renderer->FlushImpl(); }

    static void DrawQuad(const glm::vec2& InPosition, const glm::vec2& InSize, const glm::vec4& InColor);
    static void DrawQuad(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec4& InColor);

    static void DrawQuad(const glm::vec2& InPosition, const glm::vec2& InSize, const glm::vec3& InRotation, const Ref<Texture2D>& InTexture,
                         const glm::vec4& InColor = glm::vec4(1.0f));
    static void DrawQuad(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec3& InRotation, const Ref<Texture2D>& InTexture,
                         const glm::vec4& InColor = glm::vec4(1.0f));

    static void DrawRotatedQuad(const glm::vec2& InPosition, const glm::vec2& InSize, const glm::vec3& InRotation,
                                const glm::vec4& InColor);
    static void DrawRotatedQuad(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec3& InRotation,
                                const glm::vec4& InColor);

    static void DrawTexturedQuad(const glm::vec2& InPosition, const glm::vec2& InSize, const Ref<Texture2D>& InTexture,
                                 const glm::vec4& InBlendColor = glm::vec4(1.0f));
    static void DrawTexturedQuad(const glm::vec3& InPosition, const glm::vec2& InSize, const Ref<Texture2D>& InTexture,
                                 const glm::vec4& InBlendColor = glm::vec4(1.0f));

  private:
    static Renderer2D* s_Renderer;

  protected:
    Renderer2D()  = default;
    ~Renderer2D() = default;

    virtual void Create()  = 0;
    virtual void Destroy() = 0;

    virtual void BeginImpl() = 0;
    virtual void FlushImpl() = 0;

    virtual void BeginSceneImpl(const Camera& InCamera) = 0;

    virtual void DrawQuadImpl(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color) = 0;

    virtual void DrawQuadImpl(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec3& InRotation,
                              const Ref<Texture2D>& InTexture, const glm::vec4& InColor) = 0;

    virtual void DrawRotatedQuadImpl(const glm::vec3& position, const glm::vec2& size, const glm::vec3& InRotation,
                                     const glm::vec4& color) = 0;

    virtual void DrawTexturedQuadImpl(const glm::vec3& InPosition, const glm::vec2& InSize, const Ref<Texture2D>& InTexture,
                                      const glm::vec4& InBlendColor) = 0;
};
}  // namespace Eclipse