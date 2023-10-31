#pragma once

#include "Gauntlet/Core/Core.h"
#include "Gauntlet/Core/Math.h"

#include "Gauntlet/Renderer/Camera/Camera.h"

#include "Buffer.h"
#include "CoreRendererStructs.h"

namespace Gauntlet
{

class Texture2D;
class Pipeline;
class VertexBuffer;
class IndexBuffer;

struct Sprite
{
    Ref<Texture2D> Texture = nullptr;
    glm::mat4 Transform    = glm::mat4(1.0f);
    glm::vec4 Color        = glm::vec4(1.0f);
    glm::vec2 SpriteCoords = glm::vec2(0.0f);
    uint32_t Layer         = 0;
};

class Renderer2D : private Uncopyable, private Unmovable
{
  public:
    static void Init();
    static void Shutdown();

    static void Begin();
    static void Flush();

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

    static void DrawTexturedQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& textureAtlas,
                                 const glm::vec2& spriteCoords, const glm::vec2& spriteSize);
    static void DrawTexturedQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& textureAtlas,
                                 const glm::vec2& spriteCoords, const glm::vec2& spriteSize);

    static void DrawTexturedQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation,
                                 const Ref<Texture2D>& textureAtlas, const glm::vec2& spriteCoords, const glm::vec2& spriteSize);
    static void DrawTexturedQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& rotation,
                                 const Ref<Texture2D>& textureAtlas, const glm::vec2& spriteCoords, const glm::vec2& spriteSize);

    FORCEINLINE static auto& GetStorageData() { return *s_RendererStorage2D; }

  protected:
    Renderer2D()          = default;
    virtual ~Renderer2D() = default;

    // Final helper-only function
    static void DrawQuadInternal(const glm::mat4& transform, const glm::vec4& blendColor, const float textureID);
    static void FlushAndReset();

    struct RendererStorage2D
    {
        // Compile-time constants
        static constexpr uint32_t MaxQuads          = 2500;
        static constexpr uint32_t MaxVertices       = MaxQuads * 4;
        static constexpr uint32_t MaxIndices        = MaxQuads * 6;
        static constexpr uint32_t MaxTextureSlots   = 32;
        static constexpr glm::vec2 TextureCoords[4] = {glm::vec2(0.0f, 0.0f),  //
                                                       glm::vec2(1.0f, 0.0f),  //
                                                       glm::vec2(1.0f, 1.0f),  //
                                                       glm::vec2(0.0f, 1.0f)};

        static constexpr glm::vec4 QuadVertexPositions[4] = {{-0.5f, -0.5f, 0.0f, 1.0f},  //
                                                             {0.5f, -0.5f, 0.0f, 1.0f},   //
                                                             {0.5f, 0.5f, 0.0f, 1.0f},    //
                                                             {-0.5f, 0.5f, 0.0f, 1.0f}};

        static constexpr size_t DefaultVertexBufferSize = MaxVertices * sizeof(QuadVertex);

        // Quad2D Base Stuff
        BufferLayout VertexBufferLayout;
        std::array<QuadVertex*, FRAMES_IN_FLIGHT> QuadVertexBufferBase;
        std::array<QuadVertex*, FRAMES_IN_FLIGHT> QuadVertexBufferPtr;
        uint32_t CurrentFrameIndex = 0;

        std::array<std::vector<Ref<VertexBuffer>>, FRAMES_IN_FLIGHT> QuadVertexBuffers;  // Per-frame
        std::array<uint32_t, FRAMES_IN_FLIGHT> CurrentVertexBufferIndex;

        Ref<Pipeline> QuadPipeline;
        Ref<IndexBuffer> QuadIndexBuffer;
        uint32_t QuadIndexCount = 0;

        std::vector<Sprite> Sprites;

        std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
        uint32_t CurrentTextureSlotIndex = 1;  // 0 slot already tied with white texture

        // Uniform related stuff
        glm::mat4 CameraProjectionMatrix = glm::mat4(1.0f);
        MeshPushConstants PushConstants;

    } static* s_RendererStorage2D;
};
}  // namespace Gauntlet