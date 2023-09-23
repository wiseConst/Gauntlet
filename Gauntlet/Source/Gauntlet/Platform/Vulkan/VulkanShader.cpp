#include "GauntletPCH.h"
#include "VulkanShader.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanUtility.h"
#include "VulkanDescriptors.h"
#include "Gauntlet/Renderer/CoreRendererStructs.h"

#include "common/output_stream.h"

namespace Gauntlet
{

namespace ReflectionUtils
{
VkFormat ConvertSpvFormatToVulkan(const SpvReflectFormat& format)
{
    switch (format)
    {
        case SPV_REFLECT_FORMAT_UNDEFINED: return VK_FORMAT_UNDEFINED;
        case SPV_REFLECT_FORMAT_R16_UINT: return VK_FORMAT_R16_UINT;
        case SPV_REFLECT_FORMAT_R16_SINT: return VK_FORMAT_R16_SINT;
        case SPV_REFLECT_FORMAT_R16_SFLOAT: return VK_FORMAT_R16_SFLOAT;
        case SPV_REFLECT_FORMAT_R16G16_UINT: return VK_FORMAT_R16G16_UINT;
        case SPV_REFLECT_FORMAT_R16G16_SINT: return VK_FORMAT_R16G16_SINT;
        case SPV_REFLECT_FORMAT_R16G16_SFLOAT: return VK_FORMAT_R16G16_SFLOAT;
        case SPV_REFLECT_FORMAT_R16G16B16_UINT: return VK_FORMAT_R16G16B16_UINT;
        case SPV_REFLECT_FORMAT_R16G16B16_SINT: return VK_FORMAT_R16G16B16_SINT;
        case SPV_REFLECT_FORMAT_R16G16B16_SFLOAT: return VK_FORMAT_R16G16B16_SFLOAT;
        case SPV_REFLECT_FORMAT_R16G16B16A16_UINT: return VK_FORMAT_R16G16B16A16_UINT;
        case SPV_REFLECT_FORMAT_R16G16B16A16_SINT: return VK_FORMAT_R16G16B16A16_SINT;
        case SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
        case SPV_REFLECT_FORMAT_R32_UINT: return VK_FORMAT_R32_UINT;
        case SPV_REFLECT_FORMAT_R32_SINT: return VK_FORMAT_R32_SINT;
        case SPV_REFLECT_FORMAT_R32_SFLOAT: return VK_FORMAT_R32_SFLOAT;
        case SPV_REFLECT_FORMAT_R32G32_UINT: return VK_FORMAT_R32G32_UINT;
        case SPV_REFLECT_FORMAT_R32G32_SINT: return VK_FORMAT_R32G32_SINT;
        case SPV_REFLECT_FORMAT_R32G32_SFLOAT: return VK_FORMAT_R32G32_SFLOAT;
        case SPV_REFLECT_FORMAT_R32G32B32_UINT: return VK_FORMAT_R32G32B32_UINT;
        case SPV_REFLECT_FORMAT_R32G32B32_SINT: return VK_FORMAT_R32G32B32_SINT;
        case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT: return VK_FORMAT_R32G32B32_SFLOAT;
        case SPV_REFLECT_FORMAT_R32G32B32A32_UINT: return VK_FORMAT_R32G32B32A32_UINT;
        case SPV_REFLECT_FORMAT_R32G32B32A32_SINT: return VK_FORMAT_R32G32B32A32_SINT;
        case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case SPV_REFLECT_FORMAT_R64_UINT: return VK_FORMAT_R64_UINT;
        case SPV_REFLECT_FORMAT_R64_SINT: return VK_FORMAT_R64_SINT;
        case SPV_REFLECT_FORMAT_R64_SFLOAT: return VK_FORMAT_R64_SFLOAT;
        case SPV_REFLECT_FORMAT_R64G64_UINT: return VK_FORMAT_R64G64_UINT;
        case SPV_REFLECT_FORMAT_R64G64_SINT: return VK_FORMAT_R64G64_SINT;
        case SPV_REFLECT_FORMAT_R64G64_SFLOAT: return VK_FORMAT_R64G64_SFLOAT;
        case SPV_REFLECT_FORMAT_R64G64B64_UINT: return VK_FORMAT_R64G64B64_UINT;
        case SPV_REFLECT_FORMAT_R64G64B64_SINT: return VK_FORMAT_R64G64B64_SINT;
        case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT: return VK_FORMAT_R64G64B64_SFLOAT;
        case SPV_REFLECT_FORMAT_R64G64B64A64_UINT: return VK_FORMAT_R64G64B64A64_UINT;
        case SPV_REFLECT_FORMAT_R64G64B64A64_SINT: return VK_FORMAT_R64G64B64A64_SINT;
        case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT: return VK_FORMAT_R64G64B64A64_SFLOAT;
    }

    GNT_ASSERT(false, "Unknown spirv format!");
    return VK_FORMAT_UNDEFINED;
}

// TODO: Fill this completely
EShaderDataType ConvertVulkanFormatToGauntlet(const VkFormat& format)
{
    switch (format)
    {
        case VK_FORMAT_R32_SFLOAT: return EShaderDataType::FLOAT;
        case VK_FORMAT_R32G32_SFLOAT: return EShaderDataType::Vec2;
        case VK_FORMAT_R32G32B32_SFLOAT: return EShaderDataType::Vec3;
        case VK_FORMAT_R32G32B32A32_SFLOAT: return EShaderDataType::Vec4;
        case VK_FORMAT_R32G32_SINT: return EShaderDataType::Ivec2;
        case VK_FORMAT_R32G32B32A32_UINT: return EShaderDataType::Uvec4;
        case VK_FORMAT_R64_SFLOAT: return EShaderDataType::Double;
    }

    GNT_ASSERT(false, "Unknown vulkan format!");
    return EShaderDataType::None;
}

void PrintDescriptorBinding(const SpvReflectDescriptorBinding& obj)
{
    LOG_DEBUG("Set: %u    binding : %u", obj.set, obj.binding);
    LOG_DEBUG("Type: %s", ToStringDescriptorType(obj.descriptor_type).data());

    // array
    if (obj.array.dims_count > 0)
    {
        LOG_DEBUG("array   : ");
        for (uint32_t dim_index = 0; dim_index < obj.array.dims_count; ++dim_index)
        {
            LOG_DEBUG("[%u]", obj.array.dims[dim_index]);
        }
    }

    // counter
    if (obj.uav_counter_binding)
    {
        LOG_DEBUG("counter : (set=%u, binding=%u, name=%s);", obj.uav_counter_binding->set, obj.uav_counter_binding->binding,
                  obj.uav_counter_binding->name);
    }

    if ((obj.type_description->type_name) && (strlen(obj.type_description->type_name) > 0))
        LOG_DEBUG("name    : (%s): %s", obj.type_description->type_name, obj.name);
    else
        LOG_DEBUG("name    : %s", obj.name);
}

void PrintDescriptorSet(const SpvReflectDescriptorSet& obj)
{
    LOG_DEBUG("Set           : %u", obj.set);
    LOG_DEBUG("Binding count : %u", obj.binding_count);

    for (uint32_t i = 0; i < obj.binding_count; ++i)
        PrintDescriptorBinding(*obj.bindings[i]);
}

}  // namespace ReflectionUtils

VulkanShader::VulkanShader(const std::string_view& filePath)
{
    std::vector<const char*> shaderExtensions = {"vert", "frag", "geom", "comp"};
    for (auto& ext : shaderExtensions)
    {
        std::filesystem::path shaderPath(std::string(filePath) + '.' + ext + '.' + "spv");
        if (!std::filesystem::exists(shaderPath)) continue;

        const auto shaderCode = Utility::LoadDataFromDisk(shaderPath.string());
        auto& shaderStage     = m_ShaderStages.emplace_back();
        shaderStage.Module    = LoadShaderModule(shaderCode);

        LOG_DEBUG("Spirv-Reflect: %s", shaderPath.string().data());
        Reflect(shaderCode);
    }

    std::unordered_map<std::string, VkPushConstantRange> linkedPushConstants;
    std::map<uint32_t, std::unordered_map<std::string, VkDescriptorSetLayoutBinding>> linkedDescriptorSetLayoutBindings;

    for (auto& shaderStage : m_ShaderStages)
    {
        // Linking push constants
        for (auto& pushConstantBlock : shaderStage.PushConstants)
        {
            // In case we have 1 push constant block in 2 different shaders we simply add shader stage flags.
            if (linkedPushConstants.contains(pushConstantBlock.first))
                linkedPushConstants[pushConstantBlock.first].stageFlags |= pushConstantBlock.second.stageFlags;
            else
                linkedPushConstants[pushConstantBlock.first] = pushConstantBlock.second;
        }

        for (auto& descriptorSetBinding : shaderStage.DescriptorSetBindings)
        {
            for (auto& binding : descriptorSetBinding.Bindings)
            {
                // Again check if contains then simply add shader stage flags.
                if (linkedDescriptorSetLayoutBindings[descriptorSetBinding.Set].contains(binding.first))
                {
                    linkedDescriptorSetLayoutBindings[descriptorSetBinding.Set][binding.first].stageFlags |= binding.second.stageFlags;
                }
                else
                    linkedDescriptorSetLayoutBindings[descriptorSetBinding.Set][binding.first] = binding.second;
            }
        }
    }

    // Finally filling push constants
    for (auto& [name, pushConstantBlock] : linkedPushConstants)
        m_PushConstants.push_back(pushConstantBlock);

    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
    const VkDescriptorBindingFlags bindingFlag                   = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

    // Linking descriptor set layout bindings
    for (auto& descriptorSetLayoutBinding : linkedDescriptorSetLayoutBindings)
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        for (auto& [name, binding] : descriptorSetLayoutBinding.second)
        {
            bindings.push_back(binding);
        }
        std::sort(bindings.begin(), bindings.end(), [](const auto& lhs, const auto& rhs) { return lhs.binding < rhs.binding; });

        bindingFlagsInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        std::vector<VkDescriptorBindingFlags> bindingFlags(bindingFlagsInfo.bindingCount, bindingFlag);
        bindingFlagsInfo.pBindingFlags = bindingFlags.data();

        const auto descriptorSetLayoutCreateInfo =
            Utility::GetDescriptorSetLayoutCreateInfo(static_cast<uint32_t>(bindings.size()), bindings.data(), 0, &bindingFlagsInfo);

        VkDescriptorSetLayout& descriptorSetLayout = m_DescriptorSetLayouts.emplace_back();
        VK_CHECK(vkCreateDescriptorSetLayout(context.GetDevice()->GetLogicalDevice(), &descriptorSetLayoutCreateInfo, nullptr,
                                             &descriptorSetLayout),
                 "Failed to create descriptor set layout!");
    }

    // Now we don't need anything. Maybe I shouldn't clear them??
    for (auto& shaderStage : m_ShaderStages)
    {
        shaderStage.DescriptorSetBindings.clear();
        shaderStage.PushConstants.clear();
    }
}

void VulkanShader::Reflect(const std::vector<uint8_t>& shaderCode)
{
    GNT_ASSERT(m_ShaderStages.size() > 0);

    auto& reflectModule     = m_ShaderStages[m_ShaderStages.size() - 1].ReflectModule;
    SpvReflectResult result = spvReflectCreateShaderModule(shaderCode.size(), shaderCode.data(), &reflectModule);
    GNT_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

    // Log basic shader module info.
    LOG_DEBUG("Entrypoint      : %s", reflectModule.entry_point_name);
    LOG_DEBUG("Source lang     : %s", spvReflectSourceLanguage(reflectModule.source_language));
    LOG_DEBUG("Source lang ver : %u", reflectModule.source_language_version);

    // Determine shader stage by shader spirv execution model
    std::string shaderStageString;
    switch (reflectModule.spirv_execution_model)
    {
        case SpvExecutionModelVertex:
        {
            m_ShaderStages[m_ShaderStages.size() - 1].Stage = EShaderStage::SHADER_STAGE_VERTEX;
            shaderStageString                               = "VS";
            break;
        }
        case SpvExecutionModelGeometry:
        {
            m_ShaderStages[m_ShaderStages.size() - 1].Stage = EShaderStage::SHADER_STAGE_GEOMETRY;
            shaderStageString                               = "GS";
            break;
        }
        case SpvExecutionModelFragment:
        {
            m_ShaderStages[m_ShaderStages.size() - 1].Stage = EShaderStage::SHADER_STAGE_FRAGMENT;
            shaderStageString                               = "FS";
            break;
        }
        case SpvExecutionModelKernel:
        {
            m_ShaderStages[m_ShaderStages.size() - 1].Stage = EShaderStage::SHADER_STAGE_COMPUTE;
            shaderStageString                               = "CS";
            break;
        }
    }
    LOG_DEBUG("Stage           : %s", shaderStageString.data());

    // Output vars
    uint32_t count = 0;
    {
        result = spvReflectEnumerateOutputVariables(&reflectModule, &count, NULL);
        GNT_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

        std::vector<SpvReflectInterfaceVariable*> outputVars(count);
        result = spvReflectEnumerateOutputVariables(&reflectModule, &count, outputVars.data());
        GNT_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);
    }

    // PushConstants
    {
        result = spvReflectEnumeratePushConstantBlocks(&reflectModule, &count, nullptr);
        GNT_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

        std::vector<SpvReflectBlockVariable*> pushConstantBlocks(count);
        result = spvReflectEnumeratePushConstantBlocks(&reflectModule, &count, pushConstantBlocks.data());
        GNT_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

        LOG_DEBUG("PushConstant count: %u", count);
        for (uint32_t i = 0; i < pushConstantBlocks.size(); ++i)
        {
            LOG_DEBUG("PushConstants: (%s)", pushConstantBlocks[i]->name);
            LOG_DEBUG("Members: %u", pushConstantBlocks[i]->member_count);
            for (uint32_t k = 0; k < pushConstantBlocks[i]->member_count; ++k)
            {
                std::string memberInfoString;

                switch (pushConstantBlocks[i]->members[k].type_description->op)
                {
                    case (SpvOpTypeInt):
                    {
                        memberInfoString = "int";
                        break;
                    }

                    case (SpvOpTypeFloat):
                    {
                        memberInfoString = "float";
                        break;
                    }

                    case (SpvOpTypeVector):
                    {
                        memberInfoString =
                            "vec" +
                            std::to_string(pushConstantBlocks[i]->members[k].type_description->traits.numeric.vector.component_count);
                        break;
                    }

                    case (SpvOpTypeMatrix):
                    {
                        if (pushConstantBlocks[i]->members[k].type_description->traits.numeric.matrix.column_count == 4 &&
                            pushConstantBlocks[i]->members[k].type_description->traits.numeric.matrix.row_count == 4)
                        {
                            memberInfoString = "mat4";
                        }
                        else if (pushConstantBlocks[i]->members[k].type_description->traits.numeric.matrix.column_count == 3 &&
                                 pushConstantBlocks[i]->members[k].type_description->traits.numeric.matrix.row_count == 3)
                        {
                            memberInfoString = "mat3";
                        }
                        else
                        {
                            GNT_ASSERT("Unknown matrix dimension!");
                        }

                        break;
                    }
                }

                LOG_DEBUG(" %s: %s", memberInfoString.data(), pushConstantBlocks[i]->members[k].name);
            }

            // TODO: Fill here all push constant blocks to verify them from shaders.
            if (strcmp(pushConstantBlocks[i]->name, "MeshPushConstants") == 0)
            {
                GNT_ASSERT(sizeof(MeshPushConstants) == pushConstantBlocks[i]->size);
            }
            else if (strcmp(pushConstantBlocks[i]->name, "LightPushConstants") == 0)
            {
                GNT_ASSERT(sizeof(LightPushConstants) == pushConstantBlocks[i]->size);
            }
            else if (strcmp(pushConstantBlocks[i]->name, "u_LightSpaceUBO") == 0)
            {
                GNT_ASSERT(sizeof(LightPushConstants) == pushConstantBlocks[i]->size);
            }
            else if (strcmp(pushConstantBlocks[i]->name, "SkyboxPushConstants") == 0)
            {
                GNT_ASSERT(sizeof(MeshPushConstants) == pushConstantBlocks[i]->size);
            }
            else
            {
                GNT_ASSERT(false, "Unknown push constant block!");
            }

            const VkPushConstantRange pushConstantRange =
                Utility::GetPushConstantRange(Utility::GauntletShaderStageToVulkan(m_ShaderStages[m_ShaderStages.size() - 1].Stage),
                                              pushConstantBlocks[i]->size, pushConstantBlocks[i]->offset);

            m_ShaderStages[m_ShaderStages.size() - 1].PushConstants.insert(
                std::make_pair(std::string(pushConstantBlocks[i]->name), pushConstantRange));
        }
    }

    // DescriptorSets
    result = spvReflectEnumerateDescriptorSets(&reflectModule, &count, NULL);
    GNT_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

    // TODO: Make sure if it works with >1 descriptor sets.
    std::vector<SpvReflectDescriptorSet*> sets(count);
    result = spvReflectEnumerateDescriptorSets(&reflectModule, &count, sets.data());
    GNT_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

    m_ShaderStages[m_ShaderStages.size() - 1].DescriptorSetBindings.resize(sets.size());
    auto& descriptorSetBindings = m_ShaderStages[m_ShaderStages.size() - 1].DescriptorSetBindings;

    for (size_t iSet = 0; iSet < sets.size(); ++iSet)
    {
        // Get reflection set and print information.
        const SpvReflectDescriptorSet& reflectionSet = *(sets[iSet]);
        ReflectionUtils::PrintDescriptorSet(reflectionSet);

        ShaderStage::DescriptorSetLayoutData& descriptorSetLayout = descriptorSetBindings[iSet];
        descriptorSetLayout.Set                                   = reflectionSet.set;

        for (uint32_t iBinding = 0; iBinding < reflectionSet.binding_count; ++iBinding)
        {
            const SpvReflectDescriptorBinding& reflectionBinding = *(reflectionSet.bindings[iBinding]);

            descriptorSetLayout.Bindings.emplace(std::string(reflectionBinding.name), VkDescriptorSetLayoutBinding{});
            VkDescriptorSetLayoutBinding& descriptorSetLayoutBinding = descriptorSetLayout.Bindings[reflectionBinding.name];

            descriptorSetLayoutBinding.binding         = reflectionBinding.binding;
            descriptorSetLayoutBinding.descriptorType  = static_cast<VkDescriptorType>(reflectionBinding.descriptor_type);
            descriptorSetLayoutBinding.stageFlags      = static_cast<VkShaderStageFlagBits>(reflectModule.shader_stage);
            descriptorSetLayoutBinding.descriptorCount = 1;
            for (uint32_t iDim = 0; iDim < reflectionBinding.array.dims_count; ++iDim)
            {
                descriptorSetLayoutBinding.descriptorCount *= reflectionBinding.array.dims[iDim];
            }
        }
    }
}

BufferLayout VulkanShader::GetVertexBufferLayout()
{
    uint32_t i = UINT32_MAX;
    for (uint32_t k = 0; k < m_ShaderStages.size(); ++k)
    {
        if ((m_ShaderStages[k].ReflectModule.shader_stage & SPV_REFLECT_SHADER_STAGE_VERTEX_BIT) == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT)
            i = k;
    }
    GNT_ASSERT(i != UINT32_MAX, "GetVertexBufferLayout() but no vertex shader present!");

    uint32_t count          = 0;
    SpvReflectResult result = spvReflectEnumerateInputVariables(&m_ShaderStages[i].ReflectModule, &count, NULL);
    GNT_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<SpvReflectInterfaceVariable*> inputVars(count);
    result = spvReflectEnumerateInputVariables(&m_ShaderStages[i].ReflectModule, &count, inputVars.data());
    GNT_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<BufferElement> elements;
    elements.reserve(count);
    for (auto& inputVar : inputVars)
    {
        if (inputVar->built_in >= 0) continue;  // Default vars like gl_VertexIndex marked as ints > 0.

        elements.emplace_back(ReflectionUtils::ConvertVulkanFormatToGauntlet(ReflectionUtils::ConvertSpvFormatToVulkan(inputVar->format)),
                              inputVar->name);
    }

    BufferLayout layout(elements);
    return layout;
}

VkShaderModule VulkanShader::LoadShaderModule(const std::vector<uint8_t>& InShaderCode)
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    VkShaderModuleCreateInfo ShaderModuleCreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    ShaderModuleCreateInfo.codeSize                 = InShaderCode.size();
    ShaderModuleCreateInfo.pCode                    = reinterpret_cast<const uint32_t*>(InShaderCode.data());

    VkShaderModule ShaderModule = VK_NULL_HANDLE;
    VK_CHECK(vkCreateShaderModule(Context.GetDevice()->GetLogicalDevice(), &ShaderModuleCreateInfo, nullptr, &ShaderModule),
             "Failed to create shader module!");

    return ShaderModule;
}

void VulkanShader::Destroy()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    for (auto& shaderStage : m_ShaderStages)
    {
        spvReflectDestroyShaderModule(&shaderStage.ReflectModule);
        vkDestroyShaderModule(context.GetDevice()->GetLogicalDevice(), shaderStage.Module, nullptr);
    }

    for (auto& descriptorSetLayout : m_DescriptorSetLayouts)
        vkDestroyDescriptorSetLayout(context.GetDevice()->GetLogicalDevice(), descriptorSetLayout, nullptr);
}

}  // namespace Gauntlet