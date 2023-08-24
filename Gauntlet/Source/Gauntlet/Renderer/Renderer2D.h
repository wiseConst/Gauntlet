#pragma once

#include "Gauntlet/Core/Core.h"
#include "CoreRendererStructs.h"

#include "Gauntlet/Renderer/Camera/Camera.h"

namespace Gauntlet
{

class Texture2D;

class Renderer2D : private Uncopyable, private Unmovable
{
  public:
    static void Init();
    static void Shutdown();

    FORCEINLINE static void Begin() { s_Renderer->BeginImpl(); }
    FORCEINLINE static void Flush() { s_Renderer->FlushImpl(); }

    static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
    static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);

    static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& rotation, const Ref<Texture2D>& texture,
                         const glm::vec4& color = glm::vec4(1.0f));
    static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation, const Ref<Texture2D>& texture,
                         const glm::vec4& color = glm::vec4(1.0f));

    static void DrawQuad(const glm::mat4& transform, const glm::vec4& color);

    static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& rotation, const glm::vec4& color);
    static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation, const glm::vec4& color);

    static void DrawTexturedQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture,
                                 const glm::vec4& blendColor = glm::vec4(1.0f));
    static void DrawTexturedQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture,
                                 const glm::vec4& blendColor = glm::vec4(1.0f));

  private:
    static Renderer2D* s_Renderer;

  protected:
    Renderer2D()  = default;
    ~Renderer2D() = default;

    virtual void Create()  = 0;
    virtual void Destroy() = 0;

    virtual void BeginImpl() = 0;
    virtual void FlushImpl() = 0;

    virtual void DrawQuadImpl(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color) = 0;

    virtual void DrawQuadImpl(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation, const Ref<Texture2D>& texture,
                              const glm::vec4& color) = 0;

    virtual void DrawQuadImpl(const glm::mat4& transform, const glm::vec4& color) = 0;

    virtual void DrawRotatedQuadImpl(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation,
                                     const glm::vec4& color) = 0;

    virtual void DrawTexturedQuadImpl(const glm::vec3& position, const glm::vec2& InSize, const Ref<Texture2D>& texture,
                                      const glm::vec4& blendColor) = 0;
};
}  // namespace Gauntlet