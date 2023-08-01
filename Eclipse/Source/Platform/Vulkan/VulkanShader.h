#pragma once

#include "Eclipse/Renderer/Shader.h"
#include <volk/volk.h>

namespace Eclipse
{

class VulkanShader final : public Shader
{
  public:
    VulkanShader(const std::string_view& InFilePath);
    ~VulkanShader() = default;

    FORCEINLINE const auto& GetModule() const { return m_ShaderModule; }
    void DestroyModule();

  private:
    VkShaderModule m_ShaderModule;

    VkShaderModule LoadShaderModule(const std::vector<uint32_t>& InShaderCode);
};

}  // namespace Eclipse
