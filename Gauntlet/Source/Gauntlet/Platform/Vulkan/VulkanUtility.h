#pragma once

#include <volk/volk.h>
#include <vector>

#include "Gauntlet/Core/Core.h"
#include "Gauntlet/Renderer/Buffer.h"
#include "Gauntlet/Renderer/Pipeline.h"
#include "Gauntlet/Renderer/Renderer.h"
#include "Gauntlet/Utils/CoreUtils.h"

namespace Gauntlet
{

#define LOG_VULKAN_SHADER_REFLECTION 0
#define LOG_VULKAN_INFO 1
#define LOG_VMA_INFO 0

#define VK_FORCE_VALIDATION 0
#define RENDERDOC_DEBUG 1

#define VK_PREFER_IGPU 1
#define VK_RTX 0
#define VK_MESH_SHADING 0

static constexpr uint32_t GNT_VK_API_VERSION = VK_API_VERSION_1_3;

static const std::vector<const char*> s_InstanceLayers = {"VK_LAYER_KHRONOS_validation"};

// Now I assume these are supported
static const std::vector<const char*> s_DeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,            // Swapchain creation (array of images that we render into, and present to screen)
    VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME,    // For advanced GPU info
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,    // To get rid of render pass objects
    VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,  //
#if !RENDERDOC_DEBUG
    VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,  // For useful pipeline features that can be changed real-time.
#endif
#if VK_RTX
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,    // To build acceleration structures
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,      // To use vkCmdTraceRaysKHR
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,  // Required by acceleration structures
#endif
#if VK_MESH_SHADING
    VK_EXT_MESH_SHADER_EXTENSION_NAME,
#endif
};

#ifdef GNT_DEBUG
static constexpr bool s_bEnableValidationLayers = true;
#else
static constexpr bool s_bEnableValidationLayers = false;
#endif

static const char* GetStringVulkanResult(VkResult result)
{
    switch (result)
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

        case EShaderStage::SHADER_STAGE_TESSELLATION_CONTROL: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case EShaderStage::SHADER_STAGE_TESSELLATION_EVALUATION: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

        case EShaderStage::SHADER_STAGE_COMPUTE: return VK_SHADER_STAGE_COMPUTE_BIT;

        case EShaderStage::SHADER_STAGE_RAYGEN: return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        case EShaderStage::SHADER_STAGE_MISS: return VK_SHADER_STAGE_MISS_BIT_KHR;
        case EShaderStage::SHADER_STAGE_CLOSEST_HIT: return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        case EShaderStage::SHADER_STAGE_ANY_HIT: return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

        case EShaderStage::SHADER_STAGE_TASK: return VK_SHADER_STAGE_TASK_BIT_EXT;
        case EShaderStage::SHADER_STAGE_MESH: return VK_SHADER_STAGE_MESH_BIT_EXT;
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

NODISCARD static VkFormat GauntletShaderDataTypeToVulkan(EShaderDataType format)
{
    switch (format)
    {
        case EShaderDataType::FLOAT: return VK_FORMAT_R32_SFLOAT;
        case EShaderDataType::Int: return VK_FORMAT_R32_SINT;
        case EShaderDataType::Vec2: return VK_FORMAT_R32G32_SFLOAT;
        case EShaderDataType::Vec3: return VK_FORMAT_R32G32B32_SFLOAT;
        case EShaderDataType::Vec4: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case EShaderDataType::Ivec2: return VK_FORMAT_R32G32_SINT;
        case EShaderDataType::Ivec3: return VK_FORMAT_R32G32B32_SINT;
        case EShaderDataType::Ivec4: return VK_FORMAT_R32G32B32A32_SINT;
        case EShaderDataType::Uvec4: return VK_FORMAT_R32G32B32A32_UINT;
        case EShaderDataType::Double: VK_FORMAT_R64_SFLOAT;
    }

    GNT_ASSERT(false, "Unknown format!");
    return VK_FORMAT_UNDEFINED;
}

NODISCARD static VkVertexInputBindingDescription GetShaderBindingDescription(const uint32_t binding, const uint32_t stride,
                                                                             VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX)
{
    VkVertexInputBindingDescription BindingDescription = {};
    BindingDescription.binding                         = binding;
    BindingDescription.stride                          = stride;
    BindingDescription.inputRate                       = inputRate;

    return BindingDescription;
}

NODISCARD static VkVertexInputAttributeDescription GetShaderAttributeDescription(uint32_t binding, VkFormat format, uint32_t location,
                                                                                 uint32_t offset)
{
    VkVertexInputAttributeDescription AttributeDescription = {};
    AttributeDescription.binding                           = binding;
    AttributeDescription.format                            = format;
    AttributeDescription.location                          = location;
    AttributeDescription.offset                            = offset;

    return AttributeDescription;
}

NODISCARD static constexpr VkDescriptorSetLayoutBinding GetDescriptorSetLayoutBinding(const uint32_t binding,
                                                                                      const uint32_t descriptorCount,
                                                                                      VkDescriptorType descriptorType,
                                                                                      VkShaderStageFlags stageFlags,
                                                                                      VkSampler* immutableSamplers = VK_NULL_HANDLE)
{
    VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding = {};
    DescriptorSetLayoutBinding.binding                      = binding;
    DescriptorSetLayoutBinding.descriptorCount              = descriptorCount;
    DescriptorSetLayoutBinding.descriptorType               = descriptorType;
    DescriptorSetLayoutBinding.stageFlags                   = stageFlags;
    DescriptorSetLayoutBinding.pImmutableSamplers           = immutableSamplers;

    return DescriptorSetLayoutBinding;
}

NODISCARD static VkWriteDescriptorSet GetWriteDescriptorSet(VkDescriptorType descriptorType, const uint32_t binding,
                                                            VkDescriptorSet descriptorSet, const uint32_t descriptorCount,
                                                            VkDescriptorBufferInfo* bufferInfo,
                                                            VkBufferView* texelBufferView = VK_NULL_HANDLE, const uint32_t arrayElement = 0)
{
    VkWriteDescriptorSet WriteDescriptorSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    WriteDescriptorSet.descriptorType       = descriptorType;
    WriteDescriptorSet.dstBinding           = binding;
    WriteDescriptorSet.dstSet               = descriptorSet;
    WriteDescriptorSet.descriptorCount      = descriptorCount;
    WriteDescriptorSet.pBufferInfo          = bufferInfo;
    WriteDescriptorSet.pTexelBufferView     = texelBufferView;
    WriteDescriptorSet.dstArrayElement      = arrayElement;

    return WriteDescriptorSet;
}

NODISCARD static VkWriteDescriptorSet GetWriteDescriptorSet(VkDescriptorType descriptorType, const uint32_t binding,
                                                            VkDescriptorSet descriptorSet, const uint32_t descriptorCount,
                                                            VkDescriptorImageInfo* imageInfo,
                                                            VkBufferView* texelBufferView = VK_NULL_HANDLE, const uint32_t arrayElement = 0)
{
    VkWriteDescriptorSet WriteDescriptorSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    WriteDescriptorSet.descriptorType       = descriptorType;
    WriteDescriptorSet.dstBinding           = binding;
    WriteDescriptorSet.dstSet               = descriptorSet;
    WriteDescriptorSet.descriptorCount      = descriptorCount;
    WriteDescriptorSet.pImageInfo           = imageInfo;
    WriteDescriptorSet.pTexelBufferView     = texelBufferView;
    WriteDescriptorSet.dstArrayElement      = arrayElement;

    return WriteDescriptorSet;
}

NODISCARD static VkDescriptorBufferInfo GetDescriptorBufferInfo(const VkBuffer& buffer, const VkDeviceSize range,
                                                                const VkDeviceSize offset = 0)
{
    VkDescriptorBufferInfo descriptorBufferInfo = {buffer, offset, range};
    return descriptorBufferInfo;
}

NODISCARD static VkDescriptorSetAllocateInfo GetDescriptorSetAllocateInfo(const VkDescriptorPool& descriptorPool,
                                                                          const uint32_t descriptorSetCount,
                                                                          VkDescriptorSetLayout* descriptorSetLayouts)
{
    VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    DescriptorSetAllocateInfo.descriptorPool              = descriptorPool;
    DescriptorSetAllocateInfo.descriptorSetCount          = descriptorSetCount;
    DescriptorSetAllocateInfo.pSetLayouts                 = descriptorSetLayouts;

    return DescriptorSetAllocateInfo;
}

NODISCARD static VkDescriptorSetLayoutCreateInfo GetDescriptorSetLayoutCreateInfo(
    const uint32_t bindingCount, const VkDescriptorSetLayoutBinding* bindings,
    const VkDescriptorSetLayoutCreateFlags descriptorSetLayoutCreateFlags = 0, const void* pNext = nullptr)
{
    VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    DescriptorSetLayoutCreateInfo.bindingCount                    = bindingCount;
    DescriptorSetLayoutCreateInfo.pBindings                       = bindings;
    DescriptorSetLayoutCreateInfo.pNext                           = pNext;
    DescriptorSetLayoutCreateInfo.flags                           = descriptorSetLayoutCreateFlags;

    return DescriptorSetLayoutCreateInfo;
}

// MaxSetCount -  How many descriptor sets can be allocated from the pool
NODISCARD static VkDescriptorPoolCreateInfo GetDescriptorPoolCreateInfo(const uint32_t poolSizeCount, const uint32_t maxSetCount,
                                                                        const VkDescriptorPoolSize* poolSizes,
                                                                        VkDescriptorPoolCreateFlags descriptorPoolCreateFlags = 0,
                                                                        const void* pNext                                     = nullptr)
{
    VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    DescriptorPoolCreateInfo.poolSizeCount              = poolSizeCount;
    DescriptorPoolCreateInfo.maxSets                    = maxSetCount;
    DescriptorPoolCreateInfo.pPoolSizes                 = poolSizes;
    DescriptorPoolCreateInfo.flags                      = descriptorPoolCreateFlags;
    DescriptorPoolCreateInfo.pNext                      = pNext;

    return DescriptorPoolCreateInfo;
}

NODISCARD static VkPushConstantRange GetPushConstantRange(const VkShaderStageFlags shaderStages, const uint32_t size,
                                                          const uint32_t offset = 0)
{
    VkPushConstantRange PushConstants = {};
    PushConstants.size                = size;
    PushConstants.offset              = offset;
    PushConstants.stageFlags          = shaderStages;

    return PushConstants;
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