#pragma once

#include "Gauntlet/Renderer/Shader.h"
#include <volk/volk.h>

#include "spirv-reflect/spirv_reflect.h"

#include "VulkanUtility.h"
#include "Gauntlet/Renderer/Buffer.h"
#include "VulkanDescriptors.h"

namespace Gauntlet
{
// TODO: Make shader->Set("u_Albedo", albedo);

struct ShaderStage
{
    EShaderStage Stage                   = EShaderStage::SHADER_STAGE_VERTEX;
    VkShaderModule Module                = VK_NULL_HANDLE;
    SpvReflectShaderModule ReflectModule = {};

    struct DescriptorSetLayoutData
    {
        uint32_t Set = 0;                                                        // DescriptorSet it's from.
        std::unordered_map<std::string, VkDescriptorSetLayoutBinding> Bindings;  // Name and index slot
    };

    // Now I assume that if I want to have 1 push constant block in 2 different shader stages,
    // they should have the same name.
    std::unordered_map<std::string, VkPushConstantRange> PushConstants;

    // TODO: I think here I can simply store vector<map> cuz setIndex is the vector element index.
    std::vector<DescriptorSetLayoutData> DescriptorSetBindings;
};

class VulkanShader final : public Shader
{
  public:
    VulkanShader(const std::string_view& filePath);
    ~VulkanShader() = default;

    FORCEINLINE const auto& GetStages() const { return m_ShaderStages; }
    void Destroy() final override;

    BufferLayout GetVertexBufferLayout();
    FORCEINLINE const auto& GetPushConstants() const { return m_PushConstants; }
    FORCEINLINE const auto& GetDescriptorSetLayouts() const { return m_DescriptorSetLayouts; }

    void Set(const std::string& name, const Ref<Texture2D>& texture) final override;
    void Set(const std::string& name, const Ref<Image>& image) final override;

    // TODO: Add bufferSize var in AllocatedBuffer struct to prevent these.
    void Set(const std::string& name, const AllocatedBuffer& buffer, const uint64_t bufferSize, const uint64_t offset = 0) final override;

    FORCEINLINE auto& GetDescriptorSets() { return m_DescriptorSets; }

  private:
    std::vector<ShaderStage> m_ShaderStages;

    std::vector<VkPushConstantRange> m_PushConstants;
    std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
    std::vector<DescriptorSet> m_DescriptorSets;  // For each descriptor set layout

    VkShaderModule LoadShaderModule(const std::vector<uint8_t>& shaderCode);
    void Reflect(const std::vector<uint8_t>& shaderCode);

    void UpdateDescriptorSets(const std::string& name, VkWriteDescriptorSet& writeDescriptorSet);
};

}  // namespace Gauntlet
