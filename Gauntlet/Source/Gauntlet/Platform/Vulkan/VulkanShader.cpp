#include "GauntletPCH.h"
#include "VulkanShader.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanUtility.h"

namespace Gauntlet
{

VulkanShader::VulkanShader(const std::string_view& filePath)
{
    const auto ShaderCode = Utility::LoadDataFromDisk(std::string(filePath));
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

    VkShaderModuleCreateInfo ShaderModuleCreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    ShaderModuleCreateInfo.codeSize                 = InShaderCode.size();
    ShaderModuleCreateInfo.pCode                    = InShaderCode.data();

    VkShaderModule ShaderModule = VK_NULL_HANDLE;
    VK_CHECK(vkCreateShaderModule(Context.GetDevice()->GetLogicalDevice(), &ShaderModuleCreateInfo, nullptr, &ShaderModule),
             "Failed to create shader module!");

    return ShaderModule;
}

}  // namespace Gauntlet