#include "EclipsePCH.h"
#include "Renderer2D.h"

#include "RendererAPI.h"
#include "CoreRendererStructs.h"
#include "Texture.h"

#include "Eclipse/Platform/Vulkan/VulkanRenderer2D.h"

namespace Eclipse
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

    ELS_ASSERT(false, "Unknown RendererAPI!");
}

void Renderer2D::DrawQuad(const glm::vec2& InPosition, const glm::vec2& InSize, const glm::vec4& InColor)
{
    DrawQuad({InPosition.x, InPosition.y, 0.0f}, InSize, InColor);
}

void Renderer2D::DrawQuad(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec4& InColor)
{
    s_Renderer->DrawQuadImpl(InPosition, InSize, InColor);
}

void Renderer2D::DrawRotatedQuad(const glm::vec2& InPosition, const glm::vec2& InSize, const glm::vec3& InRotation,
                                 const glm::vec4& InColor)
{
    DrawRotatedQuad({InPosition.x, InPosition.y, 0.0f}, InSize, InRotation, InColor);
}

void Renderer2D::DrawRotatedQuad(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec3& InRotation,
                                 const glm::vec4& InColor)
{
    s_Renderer->DrawRotatedQuadImpl(InPosition, InSize, InRotation, InColor);
}

void Renderer2D::DrawQuad(const glm::vec2& InPosition, const glm::vec2& InSize, const glm::vec3& InRotation,
                          const Ref<Texture2D>& InTexture, const glm::vec4& InColor)
{
    DrawQuad({InPosition.x, InPosition.y, 0.0f}, InSize, InRotation, InTexture, InColor);
}

void Renderer2D::DrawQuad(const glm::vec3& InPosition, const glm::vec2& InSize, const glm::vec3& InRotation,
                          const Ref<Texture2D>& InTexture, const glm::vec4& InColor)
{
    s_Renderer->DrawQuadImpl(InPosition, InSize, InRotation, InTexture, InColor);
}

void Renderer2D::DrawTexturedQuad(const glm::vec2& InPosition, const glm::vec2& InSize, const Ref<Texture2D>& InTexture,
                                  const glm::vec4& InBlendColor)
{
    DrawTexturedQuad({InPosition.x, InPosition.y, 0.0f}, InSize, InTexture, InBlendColor);
}

void Renderer2D::DrawTexturedQuad(const glm::vec3& InPosition, const glm::vec2& InSize, const Ref<Texture2D>& InTexture,
                                  const glm::vec4& InBlendColor)
{
    s_Renderer->DrawTexturedQuadImpl(InPosition, InSize, InTexture, InBlendColor);
}

void Renderer2D::Shutdown()
{
    s_Renderer->Destroy();

    delete s_Renderer;
}

}  // namespace Eclipse