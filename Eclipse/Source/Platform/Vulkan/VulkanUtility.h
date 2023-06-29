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

enum class EShaderFormat : uint8_t
{
    FLOAT = 0,
    vec2,
    vec3,
    vec4,
    ivec2,
    uvec4,
    DOUBLE
};

static EShaderFormat ConvertShaderDataTypeToHumanFormat(EShaderDataType InType)
{
    switch (InType)
    {
        case EShaderDataType::Float: return EShaderFormat::FLOAT;
        case EShaderDataType::Float2: return EShaderFormat::vec2;
        case EShaderDataType::Float3: return EShaderFormat::vec3;
        case EShaderDataType::Float4: return EShaderFormat::vec4;
        case EShaderDataType::Int2:
            return EShaderFormat::ivec2;
            // case EShaderDataType::Int4: return uvec4;
            // case EShaderDataType::DOUBLE: DOUBLE;
    }

    ELS_ASSERT(false, "Unknown ShaderDataType!");
    return EShaderFormat::FLOAT;
}

static VkFormat ConvertHumanFormatToVulkan(EShaderFormat InFormat)
{

    switch (InFormat)
    {
        case EShaderFormat::FLOAT: return VK_FORMAT_R32_SFLOAT;
        case EShaderFormat::vec2: return VK_FORMAT_R32G32_SFLOAT;
        case EShaderFormat::vec3: return VK_FORMAT_R32G32B32_SFLOAT;
        case EShaderFormat::vec4: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case EShaderFormat::ivec2: return VK_FORMAT_R32G32_SINT;
        case EShaderFormat::uvec4: return VK_FORMAT_R32G32B32A32_UINT;
        case EShaderFormat::DOUBLE: VK_FORMAT_R64_SFLOAT;
    }

    ELS_ASSERT(false, "Unknown format!");
    return VK_FORMAT_UNDEFINED;
}

static VkVertexInputBindingDescription GetBindingDescription(uint32_t InBindingID, uint32_t InStride,
                                                             VkVertexInputRate InInputRate = VK_VERTEX_INPUT_RATE_VERTEX)
{
    VkVertexInputBindingDescription BindingDescription = {};
    BindingDescription.stride = InStride;
    BindingDescription.inputRate = InInputRate;
    BindingDescription.binding = InBindingID;

    return BindingDescription;
}

static VkVertexInputAttributeDescription GetAttributeDescription(uint32_t InBindingID, VkFormat InFormat, uint32_t InLocation,
                                                                 uint32_t InOffset)
{
    VkVertexInputAttributeDescription AttributeDescription = {};
    AttributeDescription.binding = InBindingID;
    AttributeDescription.format = InFormat;
    AttributeDescription.location = InLocation;
    AttributeDescription.offset = InOffset;

    return AttributeDescription;
}

// TODO: ImageViewSpecification structure for subresourceRange?
static void CreateImageView(const VkDevice& InDevice, const VkImage& InImage, VkImageView* InImageView, VkFormat InFormat,
                            VkImageAspectFlags InAspectFlags)
{
    VkImageViewCreateInfo ImageViewCreateInfo = {};
    ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

    ImageViewCreateInfo.image = InImage;
    ImageViewCreateInfo.format = InFormat;

    // It determines what is affected by your image operation. (In example you are using this image for depth then you set
    // VK_IMAGE_ASPECT_DEPTH_BIT)
    ImageViewCreateInfo.subresourceRange.aspectMask = InAspectFlags;

    ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    ImageViewCreateInfo.subresourceRange.levelCount = 1;
    ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    ImageViewCreateInfo.subresourceRange.layerCount = 1;

    // We don't need to swizzle ( swap around ) any of the
    // color channels
    ImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    ImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    ImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    ImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;

    VK_CHECK(vkCreateImageView(InDevice, &ImageViewCreateInfo, nullptr, InImageView), "Failed to create an image view!");
}

static VkFormat ChooseSupportedImageFormat(const VkPhysicalDevice& InDevice, const std::vector<VkFormat>& AvailableFormats,
                                           VkImageTiling InImageTiling, VkFormatFeatureFlags InFormatFeatures)
{
    for (auto& Format : AvailableFormats)
    {
        VkFormatProperties FormatProps = {};
        vkGetPhysicalDeviceFormatProperties(InDevice, Format, &FormatProps);

        if (InImageTiling == VK_IMAGE_TILING_LINEAR && (FormatProps.linearTilingFeatures & InFormatFeatures) == InFormatFeatures)
        {
            return Format;
        }
        else if (InImageTiling == VK_IMAGE_TILING_OPTIMAL && (FormatProps.optimalTilingFeatures & InFormatFeatures) == InFormatFeatures)
        {
            return Format;
        }
    }

    ELS_ASSERT(false, "Failed to find a supported format!");
    return VK_FORMAT_UNDEFINED;
}

}  // namespace Eclipse