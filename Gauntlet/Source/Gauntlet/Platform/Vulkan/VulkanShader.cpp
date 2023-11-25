#include "GauntletPCH.h"
#include "VulkanShader.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanUtility.h"
#include "VulkanDescriptors.h"
#include "VulkanTexture.h"
#include "VulkanTextureCube.h"
#include "VulkanBuffer.h"
#include "Gauntlet/Renderer/CoreRendererStructs.h"

#include "Gauntlet/Core/Timer.h"

// For compiling
#include <shaderc/shaderc.hpp>

// For refleciton
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

EShaderDataType ConvertVulkanFormatToGauntlet(const VkFormat& format)
{
    switch (format)
    {
        case VK_FORMAT_R32_SFLOAT: return EShaderDataType::FLOAT;
        case VK_FORMAT_R32_SINT: return EShaderDataType::Int;
        case VK_FORMAT_R32G32_SFLOAT: return EShaderDataType::Vec2;
        case VK_FORMAT_R32G32_SINT: return EShaderDataType::Ivec2;
        case VK_FORMAT_R32G32B32_SFLOAT: return EShaderDataType::Vec3;
        case VK_FORMAT_R32G32B32_SINT: return EShaderDataType::Ivec3;
        case VK_FORMAT_R32G32B32A32_SFLOAT: return EShaderDataType::Vec4;
        case VK_FORMAT_R32G32B32A32_SINT: return EShaderDataType::Ivec4;
        case VK_FORMAT_R32G32B32A32_UINT: return EShaderDataType::Uvec4;
        case VK_FORMAT_R64_SFLOAT: return EShaderDataType::Double;
    }

    GNT_ASSERT(false, "Unknown vulkan format!");
    return EShaderDataType::None;
}

void PrintDescriptorBinding(const SpvReflectDescriptorBinding& obj)
{
    if (!LOG_VULKAN_SHADER_REFLECTION) return;

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
    if (!LOG_VULKAN_SHADER_REFLECTION) return;

    LOG_DEBUG("Set           : %u", obj.set);
    LOG_DEBUG("Binding count : %u", obj.binding_count);

    for (uint32_t i = 0; i < obj.binding_count; ++i)
        PrintDescriptorBinding(*obj.bindings[i]);
}

}  // namespace ReflectionUtils

// TODO: Add Working Dir to application class

std::vector<uint8_t> VulkanShader::CompileOrGetSpvBinaries(const std::string& shaderExt, const std::string& filePath,
                                                           const std::string& shaderSourcePath, const std::string& shaderCachePathStr)
{
    std::vector<uint8_t> spvData;
    std::filesystem::path shaderCachePath(shaderCachePathStr + '.' + shaderExt + '.' + "spv");

    // Firstly check && try to load shader cache
    if (std::filesystem::exists(shaderCachePath)) spvData = Utility::LoadDataFromDisk(shaderCachePath.string().data());
    if (!spvData.empty()) return spvData;

    // If shader cache doesn't exist we compile.
    shaderc_shader_kind shaderType = {};
    if (shaderExt == "vert")
        shaderType = shaderc_glsl_vertex_shader;
    else if (shaderExt == "frag")
        shaderType = shaderc_glsl_fragment_shader;
    else if (shaderExt == "geom")
        shaderType = shaderc_glsl_geometry_shader;
    else if (shaderExt == "comp")
        shaderType = shaderc_glsl_compute_shader;
    else if (shaderExt == "miss")
        shaderType = shaderc_glsl_miss_shader;
    else if (shaderExt == "raygen")
        shaderType = shaderc_glsl_raygen_shader;
    else
        GNT_ASSERT(false);

    const auto compileBegin = Timer::Now();

    // Parse GLSL
    auto glslSource = Utility::LoadDataFromDisk(shaderSourcePath.data());
    const std::string glslSourceString(glslSource.begin(), glslSource.end());

    // Shaderc compiler
    shaderc::Compiler compiler      = {};
    shaderc::CompileOptions options = {};
    options.SetOptimizationLevel(shaderc_optimization_level_zero);

    // Compile GLSL into SPV binaries
    const auto compiledShader = compiler.CompileGlslToSpv(glslSourceString, shaderType, shaderSourcePath.data(), options);
    if (compiledShader.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        LOG_ERROR("SHADERC ERROR:%s", compiledShader.GetErrorMessage().data());
        GNT_ASSERT(false);
    }

    // Retrieve spv binaries and store it to  disk
    spvData = std::vector<uint8_t>((const uint8_t*)compiledShader.cbegin(), (const uint8_t*)compiledShader.cend());
    GNT_ASSERT(Utility::SaveDataToDisk(spvData.data(), spvData.size() * sizeof(spvData[0]), shaderCachePath.string()) == true);

    const auto compileEnd = Timer::Now();
    LOG_TRACE("Time took to compile %s, (%0.3f)ms", (std::string(filePath) + "." + shaderExt).data(),
              (compileEnd - compileBegin) * 1000.0f);

    return spvData;
}

VulkanShader::VulkanShader(const std::string_view& filePath)
{
    const std::string shaderCachePathStr  = "Resources/Cached/Shaders/" + std::string(filePath);
    const std::string shaderSourcePathStr = "Resources/Shaders/" + std::string(filePath);

    GNT_ASSERT(std::filesystem::is_directory("Resources/Shaders"), "Can't find shader source directory!");
    if (!std::filesystem::is_directory("Resources/Cached/Shaders")) std::filesystem::create_directories("Resources/Cached/Shaders");

    std::vector<const char*> shaderExtensions = {"vert", "frag", "geom", "comp", "miss", "raygen"};
    for (auto& ext : shaderExtensions)
    {
        std::filesystem::path shaderSourcePath(shaderSourcePathStr + '.' + ext);
        if (!std::filesystem::exists(shaderSourcePath)) continue;

        auto spvData       = CompileOrGetSpvBinaries(ext, std::string(filePath), shaderSourcePath.string(), shaderCachePathStr);
        auto& shaderStage  = m_ShaderStages.emplace_back();
        shaderStage.Module = LoadShaderModule(spvData);

        LOG_DEBUG("Spirv-Reflect: %s", shaderSourcePath.string().data());
        Reflect(spvData);
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
    const VkDescriptorBindingFlags bindingFlag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

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
            Utility::GetDescriptorSetLayoutCreateInfo(static_cast<uint32_t>(bindings.size()), bindings.data(),
                                                      VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT, &bindingFlagsInfo);

        VkDescriptorSetLayout& descriptorSetLayout = m_DescriptorSetLayouts.emplace_back();
        VK_CHECK(vkCreateDescriptorSetLayout(context.GetDevice()->GetLogicalDevice(), &descriptorSetLayoutCreateInfo, nullptr,
                                             &descriptorSetLayout),
                 "Failed to create descriptor set layout!");
    }

    for (uint32_t i = 0; i < m_DescriptorSetLayouts.size(); ++i)
    {
        DescriptorSet ds = {};
        GNT_ASSERT(context.GetDescriptorAllocator()->Allocate(ds, m_DescriptorSetLayouts[i]),
                   "Failed to allocate descriptor set for shader needs!");
        m_DescriptorSets.push_back(ds);
    }
}

void VulkanShader::Reflect(const std::vector<uint8_t>& shaderCode)
{
    GNT_ASSERT(m_ShaderStages.size() > 0);

    auto& reflectModule     = m_ShaderStages[m_ShaderStages.size() - 1].ReflectModule;
    SpvReflectResult result = spvReflectCreateShaderModule(shaderCode.size(), shaderCode.data(), &reflectModule);
    GNT_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

#if LOG_VULKAN_SHADER_REFLECTION
    // Log basic shader module info.
    LOG_DEBUG("Entrypoint      : %s", reflectModule.entry_point_name);
    LOG_DEBUG("Source lang     : %s", spvReflectSourceLanguage(reflectModule.source_language));
    LOG_DEBUG("Source lang ver : %u", reflectModule.source_language_version);
#endif

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
#if LOG_VULKAN_SHADER_REFLECTION
    LOG_DEBUG("Stage           : %s", shaderStageString.data());
#endif

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

#if LOG_VULKAN_SHADER_REFLECTION
        LOG_DEBUG("PushConstant count: %u", count);
#endif
        for (uint32_t i = 0; i < pushConstantBlocks.size(); ++i)
        {

#if LOG_VULKAN_SHADER_REFLECTION
            LOG_DEBUG("PushConstants: (%s)", pushConstantBlocks[i]->name);
            LOG_DEBUG("Members: %u", pushConstantBlocks[i]->member_count);
#endif
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

#if LOG_VULKAN_SHADER_REFLECTION
                LOG_DEBUG(" %s: %s", memberInfoString.data(), pushConstantBlocks[i]->members[k].name);
#endif
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
            descriptorSetLayoutBinding.descriptorCount = reflectionBinding.count;
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

void VulkanShader::Set(const std::string& name, const Ref<Texture2D>& texture)
{
    GNT_ASSERT(!name.empty() && texture, "Invalid parameters! VulkanShader::Set()");

    auto vulkanTexture = std::static_pointer_cast<VulkanTexture2D>(texture);
    GNT_ASSERT(vulkanTexture, "Failed to cast Texture2D to VulkanTexture2D!");

    VkWriteDescriptorSet writeDescriptorSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    writeDescriptorSet.pImageInfo           = &vulkanTexture->GetImageDescriptorInfo();
    writeDescriptorSet.descriptorCount      = 1;

    UpdateDescriptorSets(name, writeDescriptorSet);
}

void VulkanShader::Set(const std::string& name, const Ref<TextureCube>& texture)
{
    GNT_ASSERT(!name.empty() && texture, "Invalid parameters! VulkanShader::Set()");

    auto vulkanTexture = std::static_pointer_cast<VulkanTextureCube>(texture);
    GNT_ASSERT(vulkanTexture, "Failed to cast TextureCube to VulkanTextureCube!");

    VkWriteDescriptorSet writeDescriptorSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    writeDescriptorSet.pImageInfo           = &vulkanTexture->GetImageDescriptorInfo();
    writeDescriptorSet.descriptorCount      = 1;

    UpdateDescriptorSets(name, writeDescriptorSet);
}

void VulkanShader::Set(const std::string& name, const Ref<Image>& image)
{
    GNT_ASSERT(!name.empty() && image, "Invalid parameters! VulkanShader::Set()");

    auto vulkanImage = std::static_pointer_cast<VulkanImage>(image);
    GNT_ASSERT(vulkanImage, "Failed to cast Image to VulkanImage!");

    VkWriteDescriptorSet writeDescriptorSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    writeDescriptorSet.pImageInfo           = &vulkanImage->GetDescriptorInfo();
    writeDescriptorSet.descriptorCount      = 1;

    UpdateDescriptorSets(name, writeDescriptorSet);
}

void VulkanShader::Set(const std::string& name, const std::vector<Ref<Texture2D>>& textures)
{
    GNT_ASSERT(!name.empty() && !textures.empty(), "Invalid parameters! VulkanShader::Set()");

    std::vector<VkDescriptorImageInfo> imageInfos = {};
    for (uint32_t i = 0; i < textures.size(); ++i)
    {
        auto vulkanTexture = std::static_pointer_cast<VulkanTexture2D>(textures[i]);
        GNT_ASSERT(vulkanTexture, "Failed to cast Texture2D to VulkanTexture2D!");

        imageInfos.push_back(vulkanTexture->GetImageDescriptorInfo());
    }

    VkWriteDescriptorSet writeDescriptorSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    writeDescriptorSet.pImageInfo           = imageInfos.data();
    writeDescriptorSet.descriptorCount      = static_cast<uint32_t>(imageInfos.size());

    UpdateDescriptorSets(name, writeDescriptorSet);
}

void VulkanShader::Set(const std::string& name, const Ref<UniformBuffer>& uniformBuffer, const uint64_t offset)
{
    GNT_ASSERT(!name.empty() && uniformBuffer, "Invalid parameters! VulkanShader::Set()");

    auto vulkanUniformBuffer = std::static_pointer_cast<VulkanUniformBuffer>(uniformBuffer);
    GNT_ASSERT(vulkanUniformBuffer, "Failed to cast UniformBuffer to VulkanUniformBuffer!");

    auto descriptorBufferInfo = Utility::GetDescriptorBufferInfo(
        vulkanUniformBuffer->GetHandles()[GraphicsContext::Get().GetCurrentFrameIndex()].Buffer, vulkanUniformBuffer->GetSize());

    VkWriteDescriptorSet writeDescriptorSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    writeDescriptorSet.pBufferInfo          = &descriptorBufferInfo;

    UpdateDescriptorSets(name, writeDescriptorSet);
}

void VulkanShader::UpdateDescriptorSets(const std::string& name, VkWriteDescriptorSet& writeDescriptorSet)
{
    GNT_ASSERT(!m_DescriptorSets.empty(), "m_DescriptorSets.size() == 0!");

    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    // Finding descriptor set to update
    VkDescriptorSet currentVulkanDescriptorSet = VK_NULL_HANDLE;

    // Binding where it's stored
    uint32_t binding = 0;
    for (auto& shaderStage : m_ShaderStages)
    {
        for (size_t iSet = 0; iSet < shaderStage.DescriptorSetBindings.size(); ++iSet)
        {
            if (!shaderStage.DescriptorSetBindings[iSet].Bindings.contains(name)) continue;

            const auto& currentDescriptorSetBindings = shaderStage.DescriptorSetBindings[iSet].Bindings[name];
            binding                                  = currentDescriptorSetBindings.binding;

            currentVulkanDescriptorSet = m_DescriptorSets[shaderStage.DescriptorSetBindings[iSet].Set].Handle;  // Can simply search by iSet
            GNT_ASSERT(currentVulkanDescriptorSet, "Retrieved descriptor set is not valid!");

            // Here I assume that pImageInfo or pBufferInfo already specified to make this function "templated".
            writeDescriptorSet.descriptorType = currentDescriptorSetBindings.descriptorType;
            writeDescriptorSet.dstBinding     = binding;
            writeDescriptorSet.dstSet         = currentVulkanDescriptorSet;

            if (writeDescriptorSet.descriptorCount == 0) writeDescriptorSet.descriptorCount = currentDescriptorSetBindings.descriptorCount;

            break;
        }

        if (currentVulkanDescriptorSet) break;
    }

    if (currentVulkanDescriptorSet)
        vkUpdateDescriptorSets(context.GetDevice()->GetLogicalDevice(), 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);
    else
        LOG_WARN("Failed to find: %s in shader!", name.data());
}

void VulkanShader::DestroyModulesAndReflectionGarbage()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    // Once I've gathered all the needed info I can get rid of garbage left.
    for (auto& shaderStage : m_ShaderStages)
    {
        if (shaderStage.ReflectModule._internal) spvReflectDestroyShaderModule(&shaderStage.ReflectModule);

        if (shaderStage.Module)
        {
            vkDestroyShaderModule(context.GetDevice()->GetLogicalDevice(), shaderStage.Module, nullptr);
            shaderStage.Module = nullptr;
        }
    }
}

void VulkanShader::Destroy()
{
    auto& context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    // In case we forgot to delete garbage.
    DestroyModulesAndReflectionGarbage();

    for (auto& descriptorSetLayout : m_DescriptorSetLayouts)
        vkDestroyDescriptorSetLayout(context.GetDevice()->GetLogicalDevice(), descriptorSetLayout, nullptr);
}

}  // namespace Gauntlet