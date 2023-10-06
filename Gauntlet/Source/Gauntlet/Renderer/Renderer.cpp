#include "GauntletPCH.h"
#include "Renderer.h"
#include "Shader.h"
#include "Texture.h"

#include "Gauntlet/Platform/Vulkan/VulkanRenderer.h"

namespace Gauntlet
{

// Static storage
Renderer* Renderer::s_Renderer = nullptr;
std::mutex Renderer::s_ResourceAccessMutex;

Renderer::RendererStats Renderer::s_RendererStats;
Renderer::RendererSettings Renderer::s_RendererSettings;
Renderer::RendererStorage* Renderer::s_RendererStorage = new Renderer::RendererStorage();

void Renderer::Init()
{
    ShaderLibrary::Init();

    // TODO: To be refactored
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan:
        {
            s_Renderer = new VulkanRenderer();
            break;
        }
        case RendererAPI::EAPI::None:
        {
            LOG_ERROR("RendererAPI::EAPI::None!");
            GNT_ASSERT(false, "Unknown RendererAPI!");
        }
    }

    const uint32_t WhiteTexutreData = 0xffffffff;
    s_RendererStorage->WhiteTexture = Texture2D::Create(&WhiteTexutreData, sizeof(WhiteTexutreData), 1, 1);
}

void Renderer::Shutdown()
{
    ShaderLibrary::Shutdown();
    s_Renderer->Destroy();

    s_RendererStorage->WhiteTexture->Destroy();

    delete s_RendererStorage;
    s_RendererStorage = nullptr;

    delete s_Renderer;
    s_Renderer = nullptr;
}

void Renderer::Begin()
{
    s_Renderer->BeginImpl();
}

void Renderer::Flush()
{
    s_Renderer->FlushImpl();
}

}  // namespace Gauntlet