#pragma once

#include <volk/volk.h>
#include <vector>

#include "Gauntlet/Core/Core.h"
#include "Gauntlet/Renderer/Buffer.h"
#include "Gauntlet/Renderer/Pipeline.h"
#include "Gauntlet/Renderer/Renderer.h"

namespace Gauntlet
{

#define LOG_VULKAN_INFO 0
#define LOG_VMA_INFO 0
#define RENDERDOC_DEBUG 1
#define VK_PREFER_IGPU 0

static constexpr uint32_t GNT_VK_API_VERSION = VK_API_VERSION_1_3;

const std::vector<const char*> VulkanLayers     = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> DeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,          // Swapchain creation (array of images that we render into, and present to screen)
    VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME,  // For advanced GPU info
#if !RENDERDOC_DEBUG
    VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,  // For useful pipeline features that can be changed real-time.
#endif
    /*
    * https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_full_screen_exclusive.html
    "VK_EXT_full_screen_exclusive",
    "VK_KHR_get_surface_capabilities2"*/
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

#ifdef GNT_ENABLE_ASSERTS

#define VK_CHECK(x, message)                                                                                                               \
    do                                                                                                                                     \
    {                                                                                                                                      \
        const VkResult result = x;                                                                                                         \
        const std::string finalMessage =                                                                                                   \
            std::string(message) + std::string(" The result is: ") + std::string(GetStringVulkanResult(result));                           \
        if (result != VK_SUCCESS) GNT_ASSERT(false, finalMessage.data(), GetStringVulkanResult(result), __LINE__);                         \
                                                                                                                                           \
    } while (0);

#else
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
            if (LOG_VULKAN_INFO) LOG_INFO(pCallbackData->pMessage);
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

static VkPrimitiveTopology GauntletPrimitiveTopologyToVulkan(EPrimitiveTopology primitiveTopology)
{
    switch (primitiveTopology)
    {
        case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_POINT_LIST: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_LINE_LIST: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_LINE_STRIP: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_FAN: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    }

    GNT_ASSERT(false, "Unknown primitve topology flag!");
    return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
}

static VkShaderStageFlagBits GauntletShaderStageToVulkan(EShaderStage shaderStage)
{
    switch (shaderStage)
    {
        case EShaderStage::SHADER_STAGE_VERTEX: return VK_SHADER_STAGE_VERTEX_BIT;
        case EShaderStage::SHADER_STAGE_GEOMETRY: return VK_SHADER_STAGE_GEOMETRY_BIT;
        case EShaderStage::SHADER_STAGE_FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
        case EShaderStage::SHADER_STAGE_COMPUTE: return VK_SHADER_STAGE_COMPUTE_BIT;
    }

    GNT_ASSERT(false, "Unknown shader stage flag!");
    return VK_SHADER_STAGE_VERTEX_BIT;
}

static VkCullModeFlagBits GauntletCullModeToVulkan(ECullMode cullMode)
{
    switch (cullMode)
    {
        case ECullMode::CULL_MODE_NONE: return VK_CULL_MODE_NONE;
        case ECullMode::CULL_MODE_FRONT: return VK_CULL_MODE_FRONT_BIT;
        case ECullMode::CULL_MODE_BACK: return VK_CULL_MODE_BACK_BIT;
        case ECullMode::CULL_MODE_FRONT_AND_BACK: return VK_CULL_MODE_FRONT_AND_BACK;
    }

    GNT_ASSERT(false, "Unknown cull mode flag!");
    return VK_CULL_MODE_NONE;
}

static VkPolygonMode GauntletPolygonModeToVulkan(EPolygonMode polygonMode)
{
    switch (polygonMode)
    {
        case EPolygonMode::POLYGON_MODE_FILL: return VK_POLYGON_MODE_FILL;
        case EPolygonMode::POLYGON_MODE_LINE: return VK_POLYGON_MODE_LINE;
        case EPolygonMode::POLYGON_MODE_POINT: return VK_POLYGON_MODE_POINT;
        case EPolygonMode::POLYGON_MODE_FILL_RECTANGLE_NV: return VK_POLYGON_MODE_FILL_RECTANGLE_NV;
    }

    GNT_ASSERT(false, "Unknown polygon mode flag!");
    return VK_POLYGON_MODE_FILL;
}

static VkFrontFace GauntletFrontFaceToVulkan(EFrontFace frontFace)
{
    switch (frontFace)
    {
        case EFrontFace::FRONT_FACE_CLOCKWISE: return VK_FRONT_FACE_CLOCKWISE;
        case EFrontFace::FRONT_FACE_COUNTER_CLOCKWISE: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }

    GNT_ASSERT(false, "Unknown front face flag!");
    return VK_FRONT_FACE_CLOCKWISE;
}

static VkCompareOp GauntletCompareOpToVulkan(ECompareOp compareOp)
{
    switch (compareOp)
    {
        case ECompareOp::COMPARE_OP_NEVER: return VK_COMPARE_OP_NEVER;
        case ECompareOp::COMPARE_OP_LESS: return VK_COMPARE_OP_LESS;
        case ECompareOp::COMPARE_OP_EQUAL: return VK_COMPARE_OP_EQUAL;
        case ECompareOp::COMPARE_OP_LESS_OR_EQUAL: return VK_COMPARE_OP_LESS_OR_EQUAL;
        case ECompareOp::COMPARE_OP_GREATER: return VK_COMPARE_OP_GREATER;
        case ECompareOp::COMPARE_OP_NOT_EQUAL: return VK_COMPARE_OP_NOT_EQUAL;
        case ECompareOp::COMPARE_OP_GREATER_OR_EQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case ECompareOp::COMPARE_OP_ALWAYS: return VK_COMPARE_OP_ALWAYS;
    }

    GNT_ASSERT(false, "Unknown compare op flag!");
    return VK_COMPARE_OP_NEVER;
}

NODISCARD static VkCommandBuffer BeginSingleTimeCommands(const VkCommandPool& CommandPool, const VkDevice& Device)
{
    VkCommandBufferAllocateInfo AllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    AllocateInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    AllocateInfo.commandPool                 = CommandPool;
    AllocateInfo.commandBufferCount          = 1;

    VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
    VK_CHECK(vkAllocateCommandBuffers(Device, &AllocateInfo, &CommandBuffer), "Failed to allocate command buffer for single time command!");

    VkCommandBufferBeginInfo BeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    BeginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(CommandBuffer, &BeginInfo), "Failed to begin command buffer for single time command!");

    return CommandBuffer;
}

static void EndSingleTimeCommands(const VkCommandBuffer& CommandBuffer, const VkCommandPool& CommandPool, const VkQueue& Queue,
                                  const VkDevice& Device)
{
    VK_CHECK(vkEndCommandBuffer(CommandBuffer), "Failed to end command buffer for single time command!");

    VkSubmitInfo SubmitInfo       = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers    = &CommandBuffer;

    VK_CHECK(vkQueueSubmit(Queue, 1, &SubmitInfo, VK_NULL_HANDLE), "Failed to submit command buffer to the queue!");
    VK_CHECK(vkQueueWaitIdle(Queue), "Failed to wait on queue submit!");

    vkFreeCommandBuffers(Device, CommandPool, 1, &CommandBuffer);
}

NODISCARD static VkFormat GauntletShaderDataTypeToVulkan(EShaderDataType Format)
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

NODISCARD static VkVertexInputBindingDescription GetShaderBindingDescription(uint32_t BindingID, uint32_t Stride,
                                                                             VkVertexInputRate InputRate = VK_VERTEX_INPUT_RATE_VERTEX)
{
    VkVertexInputBindingDescription BindingDescription = {};
    BindingDescription.binding                         = BindingID;
    BindingDescription.stride                          = Stride;
    BindingDescription.inputRate                       = InputRate;

    return BindingDescription;
}

NODISCARD static VkVertexInputAttributeDescription GetShaderAttributeDescription(uint32_t BindingID, VkFormat Format, uint32_t Location,
                                                                                 uint32_t Offset)
{
    VkVertexInputAttributeDescription AttributeDescription = {};
    AttributeDescription.binding                           = BindingID;
    AttributeDescription.format                            = Format;
    AttributeDescription.location                          = Location;
    AttributeDescription.offset                            = Offset;

    return AttributeDescription;
}

NODISCARD static constexpr VkDescriptorSetLayoutBinding GetDescriptorSetLayoutBinding(const uint32_t Binding,
                                                                                      const uint32_t DescriptorCount,
                                                                                      VkDescriptorType DescriptorType,
                                                                                      VkShaderStageFlags StageFlags,
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

NODISCARD static VkWriteDescriptorSet GetWriteDescriptorSet(VkDescriptorType DescriptorType, const uint32_t Binding,
                                                            VkDescriptorSet DescriptorSet, const uint32_t DescriptorCount,
                                                            VkDescriptorBufferInfo* BufferInfo,
                                                            VkBufferView* TexelBufferView = VK_NULL_HANDLE, const uint32_t ArrayElement = 0)
{
    VkWriteDescriptorSet WriteDescriptorSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    WriteDescriptorSet.descriptorType       = DescriptorType;
    WriteDescriptorSet.dstBinding           = Binding;
    WriteDescriptorSet.dstSet               = DescriptorSet;
    WriteDescriptorSet.descriptorCount      = DescriptorCount;
    WriteDescriptorSet.pBufferInfo          = BufferInfo;
    WriteDescriptorSet.pTexelBufferView     = TexelBufferView;
    WriteDescriptorSet.dstArrayElement      = ArrayElement;

    return WriteDescriptorSet;
}

NODISCARD static VkWriteDescriptorSet GetWriteDescriptorSet(VkDescriptorType DescriptorType, const uint32_t Binding,
                                                            VkDescriptorSet DescriptorSet, const uint32_t DescriptorCount,
                                                            VkDescriptorImageInfo* ImageInfo,
                                                            VkBufferView* TexelBufferView = VK_NULL_HANDLE, const uint32_t ArrayElement = 0)
{
    VkWriteDescriptorSet WriteDescriptorSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    WriteDescriptorSet.descriptorType       = DescriptorType;
    WriteDescriptorSet.dstBinding           = Binding;
    WriteDescriptorSet.dstSet               = DescriptorSet;
    WriteDescriptorSet.descriptorCount      = DescriptorCount;
    WriteDescriptorSet.pImageInfo           = ImageInfo;
    WriteDescriptorSet.pTexelBufferView     = TexelBufferView;
    WriteDescriptorSet.dstArrayElement      = ArrayElement;

    return WriteDescriptorSet;
}

NODISCARD static VkDescriptorBufferInfo GetDescriptorBufferInfo(const VkBuffer& buffer, const VkDeviceSize range,
                                                                const VkDeviceSize offset = 0)
{
    VkDescriptorBufferInfo descriptorBufferInfo = {};
    descriptorBufferInfo.buffer                 = buffer;
    descriptorBufferInfo.range                  = range;
    descriptorBufferInfo.offset                 = offset;

    return descriptorBufferInfo;
}

NODISCARD static VkDescriptorSetAllocateInfo GetDescriptorSetAllocateInfo(const VkDescriptorPool& DescriptorPool,
                                                                          const uint32_t DescriptorSetCount,
                                                                          VkDescriptorSetLayout* DescriptorSetLayouts)
{
    VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    DescriptorSetAllocateInfo.descriptorPool              = DescriptorPool;
    DescriptorSetAllocateInfo.descriptorSetCount          = DescriptorSetCount;
    DescriptorSetAllocateInfo.pSetLayouts                 = DescriptorSetLayouts;

    return DescriptorSetAllocateInfo;
}

NODISCARD static VkDescriptorSetLayoutCreateInfo GetDescriptorSetLayoutCreateInfo(
    const uint32_t BindingCount, const VkDescriptorSetLayoutBinding* Bindings,
    const VkDescriptorSetLayoutCreateFlags DescriptorSetLayoutCreateFlags = 0, const void* pNext = nullptr)
{
    VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    DescriptorSetLayoutCreateInfo.bindingCount                    = BindingCount;
    DescriptorSetLayoutCreateInfo.pBindings                       = Bindings;
    DescriptorSetLayoutCreateInfo.pNext                           = pNext;
    DescriptorSetLayoutCreateInfo.flags                           = DescriptorSetLayoutCreateFlags;

    return DescriptorSetLayoutCreateInfo;
}

// MaxSetCount -  How many descriptor sets can be allocated from the pool
NODISCARD static VkDescriptorPoolCreateInfo GetDescriptorPoolCreateInfo(const uint32_t PoolSizeCount, const uint32_t MaxSetCount,
                                                                        const VkDescriptorPoolSize* PoolSizes,
                                                                        VkDescriptorPoolCreateFlags DescriptorPoolCreateFlags = 0,
                                                                        const void* pNext                                     = nullptr)
{
    VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    DescriptorPoolCreateInfo.poolSizeCount              = PoolSizeCount;
    DescriptorPoolCreateInfo.maxSets                    = MaxSetCount;
    DescriptorPoolCreateInfo.pPoolSizes                 = PoolSizes;
    DescriptorPoolCreateInfo.flags                      = DescriptorPoolCreateFlags;
    DescriptorPoolCreateInfo.pNext                      = pNext;

    return DescriptorPoolCreateInfo;
}

NODISCARD static VkPushConstantRange GetPushConstantRange(const VkShaderStageFlags ShaderStages, const uint32_t Size,
                                                          const uint32_t Offset = 0)
{
    VkPushConstantRange PushConstants = {};
    PushConstants.offset              = Offset;
    PushConstants.size                = Size;
    PushConstants.stageFlags          = ShaderStages;

    return PushConstants;
}

static std::vector<uint8_t> LoadDataFromDisk(const std::string& filePath)
{
    std::vector<uint8_t> data;

    std::ifstream in(filePath.data(), std::ios::in | std::ios::binary | std::ios::ate);
    if (!in.is_open())
    {
        LOG_WARN("Failed to load %s!", filePath.data());
        return data;
    }

    data.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(reinterpret_cast<char*>(data.data()), data.size());

    in.close();
    return data;
}

static VkBool32 DropPipelineCacheToDisk(const VkDevice& logicalDevice, const VkPipelineCache& pipelineCache,
                                        const std::string& cacheFilePath)
{
    size_t ñacheSize = 0;
    VK_CHECK(vkGetPipelineCacheData(logicalDevice, pipelineCache, &ñacheSize, nullptr), "Failed to retrieve pipeline cache data size!");

    std::vector<char> ñacheData(ñacheSize);
    VK_CHECK(vkGetPipelineCacheData(logicalDevice, pipelineCache, &ñacheSize, ñacheData.data()), "Failed to retrieve pipeline cache data!");

    if (!ñacheData.data() || ñacheSize <= 0)
    {
        LOG_WARN("Invalid cache data or size! %s", cacheFilePath.data());
        return VK_FALSE;
    }

    std::ofstream out(cacheFilePath, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!out.is_open())
    {
        LOG_WARN("Failed to create pipeline cache file! %s", cacheFilePath.data());
        return VK_FALSE;
    }

    out.write(ñacheData.data(), ñacheData.size());
    out.close();
    return VK_TRUE;
}

static VkCommandBufferInheritanceInfo GetCommandBufferInheritanceInfo(const VkRenderPass& renderPass, const VkFramebuffer& framebuffer,
                                                                      const uint32_t subpass                      = 0,
                                                                      const VkBool32 occlusionQueryEnable         = VK_FALSE,
                                                                      const VkQueryControlFlags queryControlFlags = 0,
                                                                      const VkQueryPipelineStatisticFlags queryPipelineStatisticsFlags = 0)
{
    VkCommandBufferInheritanceInfo commandBufferInheritanceInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};
    commandBufferInheritanceInfo.renderPass                     = renderPass;
    commandBufferInheritanceInfo.subpass                        = subpass;
    commandBufferInheritanceInfo.framebuffer                    = framebuffer;
    commandBufferInheritanceInfo.occlusionQueryEnable           = occlusionQueryEnable;
    commandBufferInheritanceInfo.queryFlags                     = queryControlFlags;
    commandBufferInheritanceInfo.pipelineStatistics             = queryPipelineStatisticsFlags;

    return commandBufferInheritanceInfo;
}

}  // namespace Utility

}  // namespace Gauntlet