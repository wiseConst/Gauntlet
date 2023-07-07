#include "EclipsePCH.h"
#include "Renderer2D.h"

#include "RendererAPI.h"
#include "CoreRendererStructs.h"

#include "Platform/Vulkan/VulkanRenderer2D.h"

namespace Eclipse
{
Renderer2D* Renderer2D::s_Renderer = nullptr;
Renderer2D::Renderer2DStats Renderer2D::s_Renderer2DStats;

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

void Renderer2D::Shutdown()
{
    s_Renderer->Destroy();

    delete s_Renderer;
}

}  // namespace Eclipse