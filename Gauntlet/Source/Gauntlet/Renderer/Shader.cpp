#include "GauntletPCH.h"
#include "Shader.h"

#include "RendererAPI.h"

#include "Gauntlet/Platform/Vulkan/VulkanShader.h"

namespace Gauntlet
{
Ref<Shader> Shader::CreateShader(const std::string_view& InFilePath)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan: return Ref<VulkanShader>(new VulkanShader(InFilePath));
    }

    GNT_ASSERT(false, "Unknown RendererAPI!");
    return Ref<Shader>();
}

}  // namespace Gauntlet