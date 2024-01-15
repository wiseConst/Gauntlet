#include "GauntletPCH.h"
#include "Shader.h"

#include "RendererAPI.h"

#include "Gauntlet/Platform/Vulkan/VulkanShader.h"

namespace Gauntlet
{
Ref<Shader> Shader::Create(const std::string_view& filePath)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan: return MakeRef<VulkanShader>(filePath);
    }

    GNT_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

}  // namespace Gauntlet