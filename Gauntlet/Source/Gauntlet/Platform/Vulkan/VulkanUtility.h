#pragma once

#include <volk/volk.h>
#include <vector>

#include "Gauntlet/Core/Core.h"
#include "Gauntlet/Renderer/Buffer.h"
#include "Gauntlet/Renderer/Renderer.h"

namespace Gauntlet
{

#define LOG_VULKAN_INFO 0

static constexpr uint32_t GNT_VK_API_VERSION = VK_API_VERSION_1_3;
static constexpr uint32_t FRAMES_IN_FLIGHT   = 2;

const std::vector<const char*> VulkanLayers     = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> DeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,         // Swapchain creation (array of images that we render into, and present to screen)
    VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME  // For advanced GPU info
};

#ifdef GNT_DEBUG
static constexpr bool s_bEnableValidationLayers = true;
#else
static constexpr bool s_bEnableValidationLayers = false;
#endif

static const char* GetStringVulkanResult(VkResult InResult)
{
    switch (InResult)
    {
        case VK_SUCCESS: return "VK_SUCCESS";
        case VK_NOT_READY: return "VK_NOT_READY";
        case VK_TIMEOUT: return "VK_TIMEOUT";
        case VK_EVENT_SET: return "VK_EVENT_SET";
        case VK_EVENT_RESET: return "VK_EVENT_RESET";
        case VK_INCOMPLETE: return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
    }

    GNT_ASSERT(false, "Unknown VkResult!");
    return nullptr;
}

#ifdef ELS_ENABLE_ASSERTS

#define VK_CHECK(x)                                                                                                                        \
    do                                                                                                                                     \
    {                                                                                                                                      \
        VkResult result = x;                                                                                                               \
        if (result != VK_SUCCESS)                                                                                                          \
        {                                                                                                                                  \
            GNT_ASSERT(false, "VkResult is: %s on #%u line.", GetStringVulkanResult(result), __LINE__);                                    \
        }                                                                                                                                  \
    } while (0);

#define VK_CHECK(x, message)                                                                                                               \
    do                                                                                                                                     \
    {                                                                                                                                      \
        VkResult result          = x;                                                                                                      \
        std::string FinalMessage = std::string(message) + std::string(" The result is: ") + std::string(GetStringVulkanResult(result));    \
        if (result != VK_SUCCESS)                                                                                                          \
        {                                                                                                                                  \
            GNT_ASSERT(false, FinalMessage.data(), GetStringVulkanResult(result), __LINE__);                                               \
        }                                                                                                                                  \
    } while (0);

#else
#define VK_CHECK(x) (x)
#define VK_CHECK(x, message) (x)
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
            if (LOG_VULKAN_INFO) LOG_INFO(pCallbackData->pMessage);
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

namespace Utility
{

static VkCommandBuffer BeginSingleTimeCommands(const VkCommandPool& CommandPool, const VkDevice& Device)
{
    VkCommandBufferAllocateInfo AllocateInfo = {};
    AllocateInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    AllocateInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    AllocateInfo.commandPool                 = CommandPool;
    AllocateInfo.commandBufferCount          = 1;

    VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
    VK_CHECK(vkAllocateCommandBuffers(Device, &AllocateInfo, &CommandBuffer), "Failed to allocate command buffer for single time command!");

    VkCommandBufferBeginInfo BeginInfo = {};
    BeginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    BeginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(CommandBuffer, &BeginInfo), "Failed to begin command buffer for single time command!");

    return CommandBuffer;
}

static void EndSingleTimeCommands(const VkCommandBuffer& CommandBuffer, const VkCommandPool& CommandPool, const VkQueue& Queue,
                                  const VkDevice& Device)
{
    VK_CHECK(vkEndCommandBuffer(CommandBuffer), "Failed to end command buffer for single time command!");

    VkSubmitInfo SubmitInfo       = {};
    SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers    = &CommandBuffer;

    VK_CHECK(vkQueueSubmit(Queue, 1, &SubmitInfo, VK_NULL_HANDLE), "Failed to submit command buffer to the queue!");
    VK_CHECK(vkQueueWaitIdle(Queue), "Failed to wait on queue submit!");

    vkFreeCommandBuffers(Device, CommandPool, 1, &CommandBuffer);
}

static VkFormat GauntletShaderDataTypeToVulkan(EShaderDataType Format)
{
    switch (Format)
    {
        case EShaderDataType::FLOAT: return VK_FORMAT_R32_SFLOAT;
        case EShaderDataType::Vec2: return VK_FORMAT_R32G32_SFLOAT;
        case EShaderDataType::Vec3: return VK_FORMAT_R32G32B32_SFLOAT;
        case EShaderDataType::Vec4: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case EShaderDataType::Ivec2: return VK_FORMAT_R32G32_SINT;
        case EShaderDataType::Uvec4: return VK_FORMAT_R32G32B32A32_UINT;
        case EShaderDataType::Double: VK_FORMAT_R64_SFLOAT;
    }

    GNT_ASSERT(false, "Unknown format!");
    return VK_FORMAT_UNDEFINED;
}

static VkVertexInputBindingDescription GetShaderBindingDescription(uint32_t BindingID, uint32_t Stride,
                                                                   VkVertexInputRate InputRate = VK_VERTEX_INPUT_RATE_VERTEX)
{
    VkVertexInputBindingDescription BindingDescription = {};
    BindingDescription.binding                         = BindingID;
    BindingDescription.stride                          = Stride;
    BindingDescription.inputRate                       = InputRate;

    return BindingDescription;
}

static VkVertexInputAttributeDescription GetShaderAttributeDescription(uint32_t BindingID, VkFormat Format, uint32_t Location,
                                                                       uint32_t Offset)
{
    VkVertexInputAttributeDescription AttributeDescription = {};
    AttributeDescription.binding                           = BindingID;
    AttributeDescription.format                            = Format;
    AttributeDescription.location                          = Location;
    AttributeDescription.offset                            = Offset;

    return AttributeDescription;
}

static VkDescriptorSetLayoutBinding GetDescriptorSetLayoutBinding(const uint32_t Binding, const uint32_t DescriptorCount,
                                                                  VkDescriptorType DescriptorType, VkShaderStageFlags StageFlags,
                                                                  VkSampler* ImmutableSamplers = VK_NULL_HANDLE)
{
    VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding = {};
    DescriptorSetLayoutBinding.binding                      = Binding;
    DescriptorSetLayoutBinding.descriptorCount              = DescriptorCount;
    DescriptorSetLayoutBinding.descriptorType               = DescriptorType;
    DescriptorSetLayoutBinding.stageFlags                   = StageFlags;
    DescriptorSetLayoutBinding.pImmutableSamplers           = ImmutableSamplers;

    return DescriptorSetLayoutBinding;
}

static VkWriteDescriptorSet GetWriteDescriptorSet(VkDescriptorType DescriptorType, const uint32_t Binding, VkDescriptorSet DescriptorSet,
                                                  const uint32_t DescriptorCount, VkDescriptorBufferInfo* BufferInfo,
                                                  VkBufferView* TexelBufferView = VK_NULL_HANDLE, const uint32_t ArrayElement = 0)
{
    VkWriteDescriptorSet WriteDescriptorSet = {};
    WriteDescriptorSet.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    WriteDescriptorSet.descriptorType       = DescriptorType;
    WriteDescriptorSet.dstBinding           = Binding;
    WriteDescriptorSet.dstSet               = DescriptorSet;
    WriteDescriptorSet.descriptorCount      = DescriptorCount;
    WriteDescriptorSet.pBufferInfo          = BufferInfo;
    WriteDescriptorSet.pTexelBufferView     = TexelBufferView;
    WriteDescriptorSet.dstArrayElement      = ArrayElement;

    return WriteDescriptorSet;
}

static VkWriteDescriptorSet GetWriteDescriptorSet(VkDescriptorType DescriptorType, const uint32_t Binding, VkDescriptorSet DescriptorSet,
                                                  const uint32_t DescriptorCount, VkDescriptorImageInfo* ImageInfo,
                                                  VkBufferView* TexelBufferView = VK_NULL_HANDLE, const uint32_t ArrayElement = 0)
{
    VkWriteDescriptorSet WriteDescriptorSet = {};
    WriteDescriptorSet.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    WriteDescriptorSet.descriptorType       = DescriptorType;
    WriteDescriptorSet.dstBinding           = Binding;
    WriteDescriptorSet.dstSet               = DescriptorSet;
    WriteDescriptorSet.descriptorCount      = DescriptorCount;
    WriteDescriptorSet.pImageInfo           = ImageInfo;
    WriteDescriptorSet.pTexelBufferView     = TexelBufferView;
    WriteDescriptorSet.dstArrayElement      = ArrayElement;

    return WriteDescriptorSet;
}

static VkDescriptorSetAllocateInfo GetDescriptorSetAllocateInfo(const VkDescriptorPool& DescriptorPool, const uint32_t DescriptorSetCount,
                                                                VkDescriptorSetLayout* DescriptorSetLayouts)
{
    VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = {};
    DescriptorSetAllocateInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DescriptorSetAllocateInfo.descriptorPool              = DescriptorPool;
    DescriptorSetAllocateInfo.descriptorSetCount          = DescriptorSetCount;
    DescriptorSetAllocateInfo.pSetLayouts                 = DescriptorSetLayouts;

    return DescriptorSetAllocateInfo;
}

static VkDescriptorSetLayoutCreateInfo GetDescriptorSetLayoutCreateInfo(
    const uint32_t BindingCount, const VkDescriptorSetLayoutBinding* Bindings,
    const VkDescriptorSetLayoutCreateFlags DescriptorSetLayoutCreateFlags = 0, const void* pNext = nullptr)
{
    VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = {};
    DescriptorSetLayoutCreateInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    DescriptorSetLayoutCreateInfo.bindingCount                    = BindingCount;
    DescriptorSetLayoutCreateInfo.pBindings                       = Bindings;
    DescriptorSetLayoutCreateInfo.pNext                           = pNext;
    DescriptorSetLayoutCreateInfo.flags                           = DescriptorSetLayoutCreateFlags;

    return DescriptorSetLayoutCreateInfo;
}

// InMaxSetCount -  How many descriptor sets can be allocated from the pool
//  How many things can be submitted to descriptor depending on it's type (mb it's wrong)
static VkDescriptorPoolCreateInfo GetDescriptorPoolCreateInfo(const uint32_t PoolSizeCount, const uint32_t MaxSetCount,
                                                              const VkDescriptorPoolSize* PoolSizes,
                                                              VkDescriptorPoolCreateFlags DescriptorPoolCreateFlags = 0,
                                                              const void* pNext                                     = nullptr)
{
    VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo = {};
    DescriptorPoolCreateInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    DescriptorPoolCreateInfo.poolSizeCount              = PoolSizeCount;
    DescriptorPoolCreateInfo.maxSets                    = MaxSetCount;
    DescriptorPoolCreateInfo.pPoolSizes                 = PoolSizes;
    DescriptorPoolCreateInfo.flags                      = DescriptorPoolCreateFlags;
    DescriptorPoolCreateInfo.pNext                      = pNext;

    return DescriptorPoolCreateInfo;
}

static VkPushConstantRange GetPushConstantRange(const VkShaderStageFlags ShaderStages, const uint32_t Size, const uint32_t Offset = 0)
{
    VkPushConstantRange PushConstants = {};
    PushConstants.offset              = Offset;
    PushConstants.size                = Size;
    PushConstants.stageFlags          = ShaderStages;

    return PushConstants;
}

static std::vector<uint8_t> LoadPipelineCacheFromDisk(const VkDevice& Device, const std::string& CacheFilePath)
{
    std::vector<uint8_t> CacheData;

    std::ifstream in(CacheFilePath, std::ios::in | std::ios::binary | std::ios::ate);
    if (!in.is_open())
    {
        LOG_WARN("Failed to load pipeline cache from disk: %s", CacheFilePath.data());
        return CacheData;
    }

    CacheData.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(reinterpret_cast<char*>(CacheData.data()), CacheData.size());

    in.close();
    return CacheData;
}

static VkBool32 DropPipelineCacheToDisk(const VkDevice& Device, const VkPipelineCache& PipelineCache, const std::string& CacheFilePath)
{
    size_t CacheSize = 0;
    VK_CHECK(vkGetPipelineCacheData(Device, PipelineCache, &CacheSize, nullptr), "Failed to retrieve pipeline cache data size!");

    std::vector<uint8_t> CacheData(CacheSize);
    VK_CHECK(vkGetPipelineCacheData(Device, PipelineCache, &CacheSize, CacheData.data()), "Failed to retrieve pipeline cache data!");

    if (!CacheData.data() || CacheSize <= 0)
    {
        LOG_WARN("Invalid cache data or size! %s", CacheFilePath.data());
        return VK_FALSE;
    }

    std::ofstream out(CacheFilePath, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!out.is_open())
    {
        LOG_WARN("Failed to create pipeline cache file! %s", CacheFilePath.data());
        return VK_FALSE;
    }

    out.write(reinterpret_cast<char*>(CacheData.data()), CacheData.size());
    out.close();
    return VK_TRUE;
}

}  // namespace Utility

}  // namespace Gauntlet