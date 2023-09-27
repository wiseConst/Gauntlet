#include "GauntletPCH.h"
#include "Renderer2D.h"

#include "RendererAPI.h"
#include "CoreRendererStructs.h"
#include "Texture.h"

#include "Gauntlet/Platform/Vulkan/VulkanRenderer2D.h"

namespace Gauntlet
{
Renderer2D* Renderer2D::s_Renderer = nullptr;

void Renderer2D::Init()
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            s_Renderer = new VulkanRenderer2D();
            return;
        }
        case RendererAPI::EAPI::None:
        {
            LOG_ERROR("RendererAPI::EAPI::None!");
        }
    }

    GNT_ASSERT(false, "Unknown RendererAPI!");
}

void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
{
    DrawQuad(glm::vec3(position, 0.0f), size, color);
}

void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
{
    s_Renderer->DrawQuadImpl(position, size, color);
}

void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& rotation, const glm::vec4& color)
{
    DrawRotatedQuad(glm::vec3(position, 0.0f), size, rotation, color);
}

void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation, const glm::vec4& color)
{
    s_Renderer->DrawRotatedQuadImpl(position, size, rotation, color);
}

void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& rotation, const Ref<Texture2D>& texture,
                          const glm::vec4& color)
{
    DrawQuad(glm::vec3(position, 0.0f), size, rotation, texture, color);
}

void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation, const Ref<Texture2D>& texture,
                          const glm::vec4& color)
{
    s_Renderer->DrawQuadImpl(position, size, rotation, texture, color);
}

void Renderer2D::DrawQuad(const glm::mat4& InTransform, const glm::vec4& color)
{
    s_Renderer->DrawQuadImpl(InTransform, color);
}

void Renderer2D::DrawTexturedQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture,
                                  const glm::vec4& InBlendColor)
{
    DrawTexturedQuad(glm::vec3(position, 0.0f), size, texture, InBlendColor);
}

void Renderer2D::DrawTexturedQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture,
                                  const glm::vec4& InBlendColor)
{
    s_Renderer->DrawTexturedQuadImpl(position, size, texture, InBlendColor);
}

void Renderer2D::DrawTexturedQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& textureAtlas,
                                  const glm::vec2& spriteCoords, const glm::vec2& spriteSize)
{
    DrawTexturedQuad(glm::vec3(position, 0.0f), size, glm::vec3(0.0f), textureAtlas, spriteCoords, spriteSize);
}

void Renderer2D::DrawTexturedQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& textureAtlas,
                                  const glm::vec2& spriteCoords, const glm::vec2& spriteSize)
{
    DrawTexturedQuad(position, size, glm::vec3(0.0f), textureAtlas, spriteCoords, spriteSize);
}

void Renderer2D::DrawTexturedQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& rotation,
                                  const Ref<Texture2D>& textureAtlas, const glm::vec2& spriteCoords, const glm::vec2& spriteSize)
{
    DrawTexturedQuad(glm::vec3(position, 0.0f), size, rotation, textureAtlas, spriteCoords, spriteSize);
}

void Renderer2D::DrawTexturedQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation,
                                  const Ref<Texture2D>& textureAtlas, const glm::vec2& spriteCoords, const glm::vec2& spriteSize)
{
    s_Renderer->DrawTexturedQuadImpl(position, size, rotation, textureAtlas, spriteCoords, spriteSize);
}

void Renderer2D::Shutdown()
{
    s_Renderer->Destroy();

    delete s_Renderer;
}

}  // namespace Gauntlet