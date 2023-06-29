#include "EclipsePCH.h"
#include "Renderer.h"

namespace Eclipse
{
void Renderer::Init(RendererAPI::EAPI GraphicsAPI)
{
    RendererAPI::Init(GraphicsAPI);
}

void Renderer::Shutdown() {}

}  // namespace Eclipse