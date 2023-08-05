#include "EclipsePCH.h"
#include "Shader.h"

#include "RendererAPI.h"

#include "Eclipse/Platform/Vulkan/VulkanShader.h"

namespace Eclipse
{
Ref<Shader> Shader::CreateShader(const std::string_view& InFilePath)
{
    switch (RendererAPI::Get())
    {
        case RendererAPI::EAPI::Vulkan: return Ref<VulkanShader>(new VulkanShader(InFilePath));
    }

    ELS_ASSERT(false, "Unknown RendererAPI!");
    return Ref<Shader>();
}

}  // namespace Eclipse