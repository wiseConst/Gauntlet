#include "EclipsePCH.h"
#include "VulkanShader.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanCore.h"

namespace Eclipse
{
static const std::vector<uint32_t> ReadFromeFile(const std::string_view& InFilePath)
{
    std::ifstream in(InFilePath.data(), std::ios::in | std::ios::binary);
    ELS_ASSERT(in, "Failed to open file! FilePath: %s", InFilePath.data());

    in.seekg(0, std::ios::end);
    std::vector<uint32_t> Result(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read((char*)Result.data(), Result.size());

    in.close();
    return Result;
}

VulkanShader::VulkanShader(const std::string_view& InFilePath)
{
    const auto ShaderCode = ReadFromeFile(InFilePath);
    m_ShaderModule        = LoadShaderModule(ShaderCode);
}

void VulkanShader::DestroyModule()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    vkDestroyShaderModule(Context.GetDevice()->GetLogicalDevice(), m_ShaderModule, nullptr);
}

VkShaderModule VulkanShader::LoadShaderModule(const std::vector<uint32_t>& InShaderCode)
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    ELS_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    VkShaderModuleCreateInfo ShaderModuleCreateInfo = {};
    ShaderModuleCreateInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderModuleCreateInfo.pCode                    = InShaderCode.data();
    ShaderModuleCreateInfo.codeSize                 = InShaderCode.size();

    VkShaderModule ShaderModule = VK_NULL_HANDLE;
    VK_CHECK(vkCreateShaderModule(Context.GetDevice()->GetLogicalDevice(), &ShaderModuleCreateInfo, nullptr, &ShaderModule),
             "Failed to create shader module!");

    return ShaderModule;
}

}  // namespace Eclipse