#include "GauntletPCH.h"
#include "VulkanShader.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanUtility.h"

namespace Gauntlet
{
static const std::vector<uint32_t> ReadFromeFile(const std::string_view& InFilePath)
{
    std::ifstream in(InFilePath.data(), std::ios::in | std::ios::binary);
    const auto ErrorMessage = std::string("Failed to open file! FilePath: ") + std::string(InFilePath.data());
    GNT_ASSERT(in, ErrorMessage.data());

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

void VulkanShader::Destroy()
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    vkDestroyShaderModule(Context.GetDevice()->GetLogicalDevice(), m_ShaderModule, nullptr);
}

VkShaderModule VulkanShader::LoadShaderModule(const std::vector<uint32_t>& InShaderCode)
{
    auto& Context = (VulkanContext&)VulkanContext::Get();
    GNT_ASSERT(Context.GetDevice()->IsValid(), "Vulkan device is not valid!");

    VkShaderModuleCreateInfo ShaderModuleCreateInfo = {};
    ShaderModuleCreateInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderModuleCreateInfo.pCode                    = InShaderCode.data();
    ShaderModuleCreateInfo.codeSize                 = InShaderCode.size();

    VkShaderModule ShaderModule = VK_NULL_HANDLE;
    VK_CHECK(vkCreateShaderModule(Context.GetDevice()->GetLogicalDevice(), &ShaderModuleCreateInfo, nullptr, &ShaderModule),
             "Failed to create shader module!");

    return ShaderModule;
}

}  // namespace Gauntlet