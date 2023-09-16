#pragma once

#include "Gauntlet/Renderer/Shader.h"
#include <volk/volk.h>

#include "spirv-reflect/spirv_reflect.h"

#include "VulkanUtility.h"
#include "Gauntlet/Renderer/Buffer.h"

namespace Gauntlet
{

struct ShaderStage
{
    EShaderStage Stage                   = EShaderStage::SHADER_STAGE_VERTEX;
    VkShaderModule Module                = VK_NULL_HANDLE;
    SpvReflectShaderModule ReflectModule = {};

    struct DescriptorSetLayoutData
    {
        uint32_t Set = 0;
        std::unordered_map<std::string, VkDescriptorSetLayoutBinding> Bindings;
    };

    // Now I assume that if I want to have 1 push constant block in 2 different shader stages,
    // they should have the same name.
    std::unordered_map<std::string, VkPushConstantRange> PushConstants;

    std::vector<DescriptorSetLayoutData> DescriptorSetBindings;
};

class VulkanShader final : public Shader
{
  public:
    VulkanShader(const std::string_view& filePath);
    ~VulkanShader() = default;

    FORCEINLINE const auto& GetStages() const { return m_ShaderStages; }
    void Destroy();

    BufferLayout GetVertexBufferLayout();
    FORCEINLINE const auto& GetPushConstants() const { return m_PushConstants; }
    FORCEINLINE const auto& GetDescriptorSetLayouts() const { return m_DescriptorSetLayouts; }

  private:
    std::vector<ShaderStage> m_ShaderStages;

    std::vector<VkPushConstantRange> m_PushConstants;
    std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;

    VkShaderModule LoadShaderModule(const std::vector<uint32_t>& shaderCode);
    void Reflect(const std::vector<uint32_t>& shaderCode);
};

}  // namespace Gauntlet
