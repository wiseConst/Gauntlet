#pragma once

#include <volk/volk.h>
#include <vector>

#include "Eclipse/Core/Core.h"
#include "Eclipse/Renderer/Buffer.h"

#include "VulkanCore.h"

namespace Eclipse
{
#define LOG_VULKAN_INFO 0

constexpr uint32_t ELS_VK_API_VERSION = VK_API_VERSION_1_3;
constexpr uint32_t FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> VulkanLayers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> DeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME};

#ifdef ELS_DEBUG
constexpr bool bEnableValidationLayers = true;
#else
constexpr bool bEnableValidationLayers = false;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    switch (messageSeverity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        {
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        {
            // LOG_INFO(pCallbackData->pMessage);
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        {
            LOG_WARN(pCallbackData->pMessage);
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        {
            LOG_ERROR(pCallbackData->pMessage);
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT: break;
    }

    return VK_FALSE;
}

// TODO: Refacture these 2 functions using classes
static VkCommandBuffer BeginSingleTimeCommands(const VkCommandPool& InCommandPool, const VkDevice& InDevice)
{
    VkCommandBufferAllocateInfo AllocateInfo = {};
    AllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    AllocateInfo.commandPool = InCommandPool;
    AllocateInfo.commandBufferCount = 1;

    VkCommandBuffer CommandBuffer;
    VK_CHECK(vkAllocateCommandBuffers(InDevice, &AllocateInfo, &CommandBuffer),
             "Failed to allocate command buffer for single time command!");

    VkCommandBufferBeginInfo BeginInfo = {};
    BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VK_CHECK(vkBeginCommandBuffer(CommandBuffer, &BeginInfo), "Failed to begin command buffer for single time command!");

    return CommandBuffer;
}

static void EndSingleTimeCommands(const VkCommandBuffer& InCommandBuffer, const VkCommandPool& InCommandPool, const VkQueue& InQueue,
                                  const VkDevice& InDevice)
{
    VK_CHECK(vkEndCommandBuffer(InCommandBuffer), "Failed to end command buffer for single time command!");

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &InCommandBuffer;

    VK_CHECK(vkQueueSubmit(InQueue, 1, &SubmitInfo, VK_NULL_HANDLE), "Failed to submit command buffer to the queue!");
    VK_CHECK(vkQueueWaitIdle(InQueue), "Failed to wait on queue submit!");

    vkFreeCommandBuffers(InDevice, InCommandPool, 1, &InCommandBuffer);
}

static VkFormat EclipseFormatToVulkan(EShaderDataType InFormat)
{
    switch (InFormat)
    {
        case EShaderDataType::FLOAT: return VK_FORMAT_R32_SFLOAT;
        case EShaderDataType::Vec2: return VK_FORMAT_R32G32_SFLOAT;
        case EShaderDataType::Vec3: return VK_FORMAT_R32G32B32_SFLOAT;
        case EShaderDataType::Vec4: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case EShaderDataType::Ivec2: return VK_FORMAT_R32G32_SINT;
        case EShaderDataType::Uvec4: return VK_FORMAT_R32G32B32A32_UINT;
        case EShaderDataType::Double: VK_FORMAT_R64_SFLOAT;
    }

    ELS_ASSERT(false, "Unknown format!");
    return VK_FORMAT_UNDEFINED;
}

static VkVertexInputBindingDescription GetShaderBindingDescription(uint32_t InBindingID, uint32_t InStride,
                                                                   VkVertexInputRate InInputRate = VK_VERTEX_INPUT_RATE_VERTEX)
{
    VkVertexInputBindingDescription BindingDescription = {};
    BindingDescription.stride = InStride;
    BindingDescription.inputRate = InInputRate;
    BindingDescription.binding = InBindingID;

    return BindingDescription;
}

static VkVertexInputAttributeDescription GetShaderAttributeDescription(uint32_t InBindingID, VkFormat InFormat, uint32_t InLocation,
                                                                       uint32_t InOffset)
{
    VkVertexInputAttributeDescription AttributeDescription = {};
    AttributeDescription.binding = InBindingID;
    AttributeDescription.format = InFormat;
    AttributeDescription.location = InLocation;
    AttributeDescription.offset = InOffset;

    return AttributeDescription;
}

}  // namespace Eclipse