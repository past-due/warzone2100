
#pragma once

#include <vulkan/vulkan.h>

//static_assert( VK_HEADER_VERSION ==  108 , "Wrong VK_HEADER_VERSION!" );

namespace WZ_vk
{
  // This is just the DispatchLoaderDynamic pulled from vulkan.hpp (1.1.108)
  class DispatchLoaderDynamic
  {
  public:
    PFN_vkCreateInstance vkCreateInstance = 0;
    PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties = 0;
    PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties = 0;
    PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion = 0;
    PFN_vkBeginCommandBuffer vkBeginCommandBuffer = 0;
    PFN_vkCmdBeginConditionalRenderingEXT vkCmdBeginConditionalRenderingEXT = 0;
    PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = 0;
    PFN_vkCmdBeginQuery vkCmdBeginQuery = 0;
#if defined(VK_EXT_transform_feedback)
    PFN_vkCmdBeginQueryIndexedEXT vkCmdBeginQueryIndexedEXT = 0;
#endif
    PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass = 0;
    PFN_vkCmdBeginRenderPass2KHR vkCmdBeginRenderPass2KHR = 0;
#if defined(VK_EXT_transform_feedback)
    PFN_vkCmdBeginTransformFeedbackEXT vkCmdBeginTransformFeedbackEXT = 0;
#endif
    PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets = 0;
    PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer = 0;
    PFN_vkCmdBindPipeline vkCmdBindPipeline = 0;
#if defined(VK_NV_shading_rate_image)
    PFN_vkCmdBindShadingRateImageNV vkCmdBindShadingRateImageNV = 0;
#endif
#if defined(VK_EXT_transform_feedback)
    PFN_vkCmdBindTransformFeedbackBuffersEXT vkCmdBindTransformFeedbackBuffersEXT = 0;
#endif
    PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers = 0;
    PFN_vkCmdBlitImage vkCmdBlitImage = 0;
#if defined(VK_NV_ray_tracing)
    PFN_vkCmdBuildAccelerationStructureNV vkCmdBuildAccelerationStructureNV = 0;
#endif
    PFN_vkCmdClearAttachments vkCmdClearAttachments = 0;
    PFN_vkCmdClearColorImage vkCmdClearColorImage = 0;
    PFN_vkCmdClearDepthStencilImage vkCmdClearDepthStencilImage = 0;
#if defined(VK_NV_ray_tracing)
    PFN_vkCmdCopyAccelerationStructureNV vkCmdCopyAccelerationStructureNV = 0;
#endif
    PFN_vkCmdCopyBuffer vkCmdCopyBuffer = 0;
    PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage = 0;
    PFN_vkCmdCopyImage vkCmdCopyImage = 0;
    PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer = 0;
    PFN_vkCmdCopyQueryPoolResults vkCmdCopyQueryPoolResults = 0;
    PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBeginEXT = 0;
    PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEndEXT = 0;
    PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsertEXT = 0;
    PFN_vkCmdDispatch vkCmdDispatch = 0;
    PFN_vkCmdDispatchBase vkCmdDispatchBase = 0;
    PFN_vkCmdDispatchBaseKHR vkCmdDispatchBaseKHR = 0;
    PFN_vkCmdDispatchIndirect vkCmdDispatchIndirect = 0;
    PFN_vkCmdDraw vkCmdDraw = 0;
    PFN_vkCmdDrawIndexed vkCmdDrawIndexed = 0;
    PFN_vkCmdDrawIndexedIndirect vkCmdDrawIndexedIndirect = 0;
    PFN_vkCmdDrawIndexedIndirectCountAMD vkCmdDrawIndexedIndirectCountAMD = 0;
#if defined(VK_KHR_draw_indirect_count)
    PFN_vkCmdDrawIndexedIndirectCountKHR vkCmdDrawIndexedIndirectCountKHR = 0;
#endif
    PFN_vkCmdDrawIndirect vkCmdDrawIndirect = 0;
#if defined(VK_EXT_transform_feedback)
    PFN_vkCmdDrawIndirectByteCountEXT vkCmdDrawIndirectByteCountEXT = 0;
#endif
    PFN_vkCmdDrawIndirectCountAMD vkCmdDrawIndirectCountAMD = 0;
#if defined(VK_KHR_draw_indirect_count)
    PFN_vkCmdDrawIndirectCountKHR vkCmdDrawIndirectCountKHR = 0;
#endif
#if defined(VK_NV_mesh_shader)
    PFN_vkCmdDrawMeshTasksIndirectCountNV vkCmdDrawMeshTasksIndirectCountNV = 0;
    PFN_vkCmdDrawMeshTasksIndirectNV vkCmdDrawMeshTasksIndirectNV = 0;
    PFN_vkCmdDrawMeshTasksNV vkCmdDrawMeshTasksNV = 0;
#endif
    PFN_vkCmdEndConditionalRenderingEXT vkCmdEndConditionalRenderingEXT = 0;
    PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = 0;
    PFN_vkCmdEndQuery vkCmdEndQuery = 0;
#if defined(VK_EXT_transform_feedback)
    PFN_vkCmdEndQueryIndexedEXT vkCmdEndQueryIndexedEXT = 0;
#endif
    PFN_vkCmdEndRenderPass vkCmdEndRenderPass = 0;
    PFN_vkCmdEndRenderPass2KHR vkCmdEndRenderPass2KHR = 0;
#if defined(VK_EXT_transform_feedback)
    PFN_vkCmdEndTransformFeedbackEXT vkCmdEndTransformFeedbackEXT = 0;
#endif
    PFN_vkCmdExecuteCommands vkCmdExecuteCommands = 0;
    PFN_vkCmdFillBuffer vkCmdFillBuffer = 0;
    PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT = 0;
    PFN_vkCmdNextSubpass vkCmdNextSubpass = 0;
    PFN_vkCmdNextSubpass2KHR vkCmdNextSubpass2KHR = 0;
    PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier = 0;
    PFN_vkCmdProcessCommandsNVX vkCmdProcessCommandsNVX = 0;
    PFN_vkCmdPushConstants vkCmdPushConstants = 0;
    PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR = 0;
    PFN_vkCmdPushDescriptorSetWithTemplateKHR vkCmdPushDescriptorSetWithTemplateKHR = 0;
    PFN_vkCmdReserveSpaceForCommandsNVX vkCmdReserveSpaceForCommandsNVX = 0;
    PFN_vkCmdResetEvent vkCmdResetEvent = 0;
    PFN_vkCmdResetQueryPool vkCmdResetQueryPool = 0;
    PFN_vkCmdResolveImage vkCmdResolveImage = 0;
    PFN_vkCmdSetBlendConstants vkCmdSetBlendConstants = 0;
    PFN_vkCmdSetCheckpointNV vkCmdSetCheckpointNV = 0;
#if defined(VK_NV_shading_rate_image)
    PFN_vkCmdSetCoarseSampleOrderNV vkCmdSetCoarseSampleOrderNV = 0;
#endif
    PFN_vkCmdSetDepthBias vkCmdSetDepthBias = 0;
    PFN_vkCmdSetDepthBounds vkCmdSetDepthBounds = 0;
    PFN_vkCmdSetDeviceMask vkCmdSetDeviceMask = 0;
    PFN_vkCmdSetDeviceMaskKHR vkCmdSetDeviceMaskKHR = 0;
    PFN_vkCmdSetDiscardRectangleEXT vkCmdSetDiscardRectangleEXT = 0;
    PFN_vkCmdSetEvent vkCmdSetEvent = 0;
#if defined(VK_NV_scissor_exclusive)
    PFN_vkCmdSetExclusiveScissorNV vkCmdSetExclusiveScissorNV = 0;
#endif
    PFN_vkCmdSetLineWidth vkCmdSetLineWidth = 0;
    PFN_vkCmdSetSampleLocationsEXT vkCmdSetSampleLocationsEXT = 0;
    PFN_vkCmdSetScissor vkCmdSetScissor = 0;
    PFN_vkCmdSetStencilCompareMask vkCmdSetStencilCompareMask = 0;
    PFN_vkCmdSetStencilReference vkCmdSetStencilReference = 0;
    PFN_vkCmdSetStencilWriteMask vkCmdSetStencilWriteMask = 0;
    PFN_vkCmdSetViewport vkCmdSetViewport = 0;
#if defined(VK_NV_shading_rate_image)
    PFN_vkCmdSetViewportShadingRatePaletteNV vkCmdSetViewportShadingRatePaletteNV = 0;
#endif
    PFN_vkCmdSetViewportWScalingNV vkCmdSetViewportWScalingNV = 0;
#if defined(VK_NV_ray_tracing)
    PFN_vkCmdTraceRaysNV vkCmdTraceRaysNV = 0;
#endif
    PFN_vkCmdUpdateBuffer vkCmdUpdateBuffer = 0;
    PFN_vkCmdWaitEvents vkCmdWaitEvents = 0;
#if defined(VK_NV_ray_tracing)
    PFN_vkCmdWriteAccelerationStructuresPropertiesNV vkCmdWriteAccelerationStructuresPropertiesNV = 0;
#endif
    PFN_vkCmdWriteBufferMarkerAMD vkCmdWriteBufferMarkerAMD = 0;
    PFN_vkCmdWriteTimestamp vkCmdWriteTimestamp = 0;
    PFN_vkEndCommandBuffer vkEndCommandBuffer = 0;
    PFN_vkResetCommandBuffer vkResetCommandBuffer = 0;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkAcquireFullScreenExclusiveModeEXT vkAcquireFullScreenExclusiveModeEXT = 0;
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
    PFN_vkAcquireNextImage2KHR vkAcquireNextImage2KHR = 0;
    PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR = 0;
    PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers = 0;
    PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets = 0;
    PFN_vkAllocateMemory vkAllocateMemory = 0;
#if defined(VK_NV_ray_tracing)
    PFN_vkBindAccelerationStructureMemoryNV vkBindAccelerationStructureMemoryNV = 0;
#endif
    PFN_vkBindBufferMemory vkBindBufferMemory = 0;
    PFN_vkBindBufferMemory2 vkBindBufferMemory2 = 0;
    PFN_vkBindBufferMemory2KHR vkBindBufferMemory2KHR = 0;
    PFN_vkBindImageMemory vkBindImageMemory = 0;
    PFN_vkBindImageMemory2 vkBindImageMemory2 = 0;
    PFN_vkBindImageMemory2KHR vkBindImageMemory2KHR = 0;
#if defined(VK_NV_ray_tracing)
    PFN_vkCompileDeferredNV vkCompileDeferredNV = 0;
    PFN_vkCreateAccelerationStructureNV vkCreateAccelerationStructureNV = 0;
#endif
    PFN_vkCreateBuffer vkCreateBuffer = 0;
    PFN_vkCreateBufferView vkCreateBufferView = 0;
    PFN_vkCreateCommandPool vkCreateCommandPool = 0;
    PFN_vkCreateComputePipelines vkCreateComputePipelines = 0;
    PFN_vkCreateDescriptorPool vkCreateDescriptorPool = 0;
    PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout = 0;
    PFN_vkCreateDescriptorUpdateTemplate vkCreateDescriptorUpdateTemplate = 0;
    PFN_vkCreateDescriptorUpdateTemplateKHR vkCreateDescriptorUpdateTemplateKHR = 0;
    PFN_vkCreateEvent vkCreateEvent = 0;
    PFN_vkCreateFence vkCreateFence = 0;
    PFN_vkCreateFramebuffer vkCreateFramebuffer = 0;
    PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines = 0;
    PFN_vkCreateImage vkCreateImage = 0;
    PFN_vkCreateImageView vkCreateImageView = 0;
    PFN_vkCreateIndirectCommandsLayoutNVX vkCreateIndirectCommandsLayoutNVX = 0;
    PFN_vkCreateObjectTableNVX vkCreateObjectTableNVX = 0;
    PFN_vkCreatePipelineCache vkCreatePipelineCache = 0;
    PFN_vkCreatePipelineLayout vkCreatePipelineLayout = 0;
    PFN_vkCreateQueryPool vkCreateQueryPool = 0;
#if defined(VK_NV_ray_tracing)
    PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelinesNV = 0;
#endif
    PFN_vkCreateRenderPass vkCreateRenderPass = 0;
    PFN_vkCreateRenderPass2KHR vkCreateRenderPass2KHR = 0;
    PFN_vkCreateSampler vkCreateSampler = 0;
    PFN_vkCreateSamplerYcbcrConversion vkCreateSamplerYcbcrConversion = 0;
    PFN_vkCreateSamplerYcbcrConversionKHR vkCreateSamplerYcbcrConversionKHR = 0;
    PFN_vkCreateSemaphore vkCreateSemaphore = 0;
    PFN_vkCreateShaderModule vkCreateShaderModule = 0;
    PFN_vkCreateSharedSwapchainsKHR vkCreateSharedSwapchainsKHR = 0;
    PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = 0;
    PFN_vkCreateValidationCacheEXT vkCreateValidationCacheEXT = 0;
    PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectNameEXT = 0;
    PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTagEXT = 0;
#if defined(VK_NV_ray_tracing)
    PFN_vkDestroyAccelerationStructureNV vkDestroyAccelerationStructureNV = 0;
#endif
    PFN_vkDestroyBuffer vkDestroyBuffer = 0;
    PFN_vkDestroyBufferView vkDestroyBufferView = 0;
    PFN_vkDestroyCommandPool vkDestroyCommandPool = 0;
    PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool = 0;
    PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout = 0;
    PFN_vkDestroyDescriptorUpdateTemplate vkDestroyDescriptorUpdateTemplate = 0;
    PFN_vkDestroyDescriptorUpdateTemplateKHR vkDestroyDescriptorUpdateTemplateKHR = 0;
    PFN_vkDestroyDevice vkDestroyDevice = 0;
    PFN_vkDestroyEvent vkDestroyEvent = 0;
    PFN_vkDestroyFence vkDestroyFence = 0;
    PFN_vkDestroyFramebuffer vkDestroyFramebuffer = 0;
    PFN_vkDestroyImage vkDestroyImage = 0;
    PFN_vkDestroyImageView vkDestroyImageView = 0;
    PFN_vkDestroyIndirectCommandsLayoutNVX vkDestroyIndirectCommandsLayoutNVX = 0;
    PFN_vkDestroyObjectTableNVX vkDestroyObjectTableNVX = 0;
    PFN_vkDestroyPipeline vkDestroyPipeline = 0;
    PFN_vkDestroyPipelineCache vkDestroyPipelineCache = 0;
    PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout = 0;
    PFN_vkDestroyQueryPool vkDestroyQueryPool = 0;
    PFN_vkDestroyRenderPass vkDestroyRenderPass = 0;
    PFN_vkDestroySampler vkDestroySampler = 0;
    PFN_vkDestroySamplerYcbcrConversion vkDestroySamplerYcbcrConversion = 0;
    PFN_vkDestroySamplerYcbcrConversionKHR vkDestroySamplerYcbcrConversionKHR = 0;
    PFN_vkDestroySemaphore vkDestroySemaphore = 0;
    PFN_vkDestroyShaderModule vkDestroyShaderModule = 0;
    PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR = 0;
    PFN_vkDestroyValidationCacheEXT vkDestroyValidationCacheEXT = 0;
    PFN_vkDeviceWaitIdle vkDeviceWaitIdle = 0;
    PFN_vkDisplayPowerControlEXT vkDisplayPowerControlEXT = 0;
    PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges = 0;
    PFN_vkFreeCommandBuffers vkFreeCommandBuffers = 0;
    PFN_vkFreeDescriptorSets vkFreeDescriptorSets = 0;
    PFN_vkFreeMemory vkFreeMemory = 0;
#if defined(VK_NV_ray_tracing)
    PFN_vkGetAccelerationStructureHandleNV vkGetAccelerationStructureHandleNV = 0;
    PFN_vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirementsNV = 0;
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
    PFN_vkGetAndroidHardwareBufferPropertiesANDROID vkGetAndroidHardwareBufferPropertiesANDROID = 0;
#endif
#endif /*VK_USE_PLATFORM_ANDROID_KHR*/
#if defined(VK_EXT_buffer_device_address)
    PFN_vkGetBufferDeviceAddressEXT vkGetBufferDeviceAddressEXT = 0;
#endif
    PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements = 0;
    PFN_vkGetBufferMemoryRequirements2 vkGetBufferMemoryRequirements2 = 0;
    PFN_vkGetBufferMemoryRequirements2KHR vkGetBufferMemoryRequirements2KHR = 0;
#if defined(VK_EXT_calibrated_timestamps)
    PFN_vkGetCalibratedTimestampsEXT vkGetCalibratedTimestampsEXT = 0;
#endif
    PFN_vkGetDescriptorSetLayoutSupport vkGetDescriptorSetLayoutSupport = 0;
#if defined(VK_KHR_maintenance3)
    PFN_vkGetDescriptorSetLayoutSupportKHR vkGetDescriptorSetLayoutSupportKHR = 0;
#endif
    PFN_vkGetDeviceGroupPeerMemoryFeatures vkGetDeviceGroupPeerMemoryFeatures = 0;
    PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR vkGetDeviceGroupPeerMemoryFeaturesKHR = 0;
    PFN_vkGetDeviceGroupPresentCapabilitiesKHR vkGetDeviceGroupPresentCapabilitiesKHR = 0;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkGetDeviceGroupSurfacePresentModes2EXT vkGetDeviceGroupSurfacePresentModes2EXT = 0;
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
    PFN_vkGetDeviceGroupSurfacePresentModesKHR vkGetDeviceGroupSurfacePresentModesKHR = 0;
    PFN_vkGetDeviceMemoryCommitment vkGetDeviceMemoryCommitment = 0;
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = 0;
    PFN_vkGetDeviceQueue vkGetDeviceQueue = 0;
    PFN_vkGetDeviceQueue2 vkGetDeviceQueue2 = 0;
    PFN_vkGetEventStatus vkGetEventStatus = 0;
    PFN_vkGetFenceFdKHR vkGetFenceFdKHR = 0;
    PFN_vkGetFenceStatus vkGetFenceStatus = 0;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkGetFenceWin32HandleKHR vkGetFenceWin32HandleKHR = 0;
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
#if defined(VK_EXT_image_drm_format_modifier)
    PFN_vkGetImageDrmFormatModifierPropertiesEXT vkGetImageDrmFormatModifierPropertiesEXT = 0;
#endif
    PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements = 0;
    PFN_vkGetImageMemoryRequirements2 vkGetImageMemoryRequirements2 = 0;
    PFN_vkGetImageMemoryRequirements2KHR vkGetImageMemoryRequirements2KHR = 0;
    PFN_vkGetImageSparseMemoryRequirements vkGetImageSparseMemoryRequirements = 0;
    PFN_vkGetImageSparseMemoryRequirements2 vkGetImageSparseMemoryRequirements2 = 0;
    PFN_vkGetImageSparseMemoryRequirements2KHR vkGetImageSparseMemoryRequirements2KHR = 0;
    PFN_vkGetImageSubresourceLayout vkGetImageSubresourceLayout = 0;
#if defined(VK_NVX_image_view_handle)
    PFN_vkGetImageViewHandleNVX vkGetImageViewHandleNVX = 0;
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
    PFN_vkGetMemoryAndroidHardwareBufferANDROID vkGetMemoryAndroidHardwareBufferANDROID = 0;
#endif
#endif /*VK_USE_PLATFORM_ANDROID_KHR*/
    PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR = 0;
    PFN_vkGetMemoryFdPropertiesKHR vkGetMemoryFdPropertiesKHR = 0;
    PFN_vkGetMemoryHostPointerPropertiesEXT vkGetMemoryHostPointerPropertiesEXT = 0;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR = 0;
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkGetMemoryWin32HandleNV vkGetMemoryWin32HandleNV = 0;
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkGetMemoryWin32HandlePropertiesKHR vkGetMemoryWin32HandlePropertiesKHR = 0;
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
    PFN_vkGetPastPresentationTimingGOOGLE vkGetPastPresentationTimingGOOGLE = 0;
    PFN_vkGetPipelineCacheData vkGetPipelineCacheData = 0;
    PFN_vkGetQueryPoolResults vkGetQueryPoolResults = 0;
#if defined(VK_NV_ray_tracing)
    PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandlesNV = 0;
#endif
    PFN_vkGetRefreshCycleDurationGOOGLE vkGetRefreshCycleDurationGOOGLE = 0;
    PFN_vkGetRenderAreaGranularity vkGetRenderAreaGranularity = 0;
    PFN_vkGetSemaphoreFdKHR vkGetSemaphoreFdKHR = 0;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkGetSemaphoreWin32HandleKHR vkGetSemaphoreWin32HandleKHR = 0;
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
    PFN_vkGetShaderInfoAMD vkGetShaderInfoAMD = 0;
    PFN_vkGetSwapchainCounterEXT vkGetSwapchainCounterEXT = 0;
    PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = 0;
    PFN_vkGetSwapchainStatusKHR vkGetSwapchainStatusKHR = 0;
    PFN_vkGetValidationCacheDataEXT vkGetValidationCacheDataEXT = 0;
    PFN_vkImportFenceFdKHR vkImportFenceFdKHR = 0;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkImportFenceWin32HandleKHR vkImportFenceWin32HandleKHR = 0;
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
    PFN_vkImportSemaphoreFdKHR vkImportSemaphoreFdKHR = 0;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkImportSemaphoreWin32HandleKHR vkImportSemaphoreWin32HandleKHR = 0;
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
    PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges = 0;
    PFN_vkMapMemory vkMapMemory = 0;
    PFN_vkMergePipelineCaches vkMergePipelineCaches = 0;
    PFN_vkMergeValidationCachesEXT vkMergeValidationCachesEXT = 0;
    PFN_vkRegisterDeviceEventEXT vkRegisterDeviceEventEXT = 0;
    PFN_vkRegisterDisplayEventEXT vkRegisterDisplayEventEXT = 0;
    PFN_vkRegisterObjectsNVX vkRegisterObjectsNVX = 0;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkReleaseFullScreenExclusiveModeEXT vkReleaseFullScreenExclusiveModeEXT = 0;
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
    PFN_vkResetCommandPool vkResetCommandPool = 0;
    PFN_vkResetDescriptorPool vkResetDescriptorPool = 0;
    PFN_vkResetEvent vkResetEvent = 0;
    PFN_vkResetFences vkResetFences = 0;
#if defined(VK_EXT_host_query_reset)
    PFN_vkResetQueryPoolEXT vkResetQueryPoolEXT = 0;
#endif
    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = 0;
    PFN_vkSetDebugUtilsObjectTagEXT vkSetDebugUtilsObjectTagEXT = 0;
    PFN_vkSetEvent vkSetEvent = 0;
    PFN_vkSetHdrMetadataEXT vkSetHdrMetadataEXT = 0;
#if defined(VK_AMD_display_native_hdr)
    PFN_vkSetLocalDimmingAMD vkSetLocalDimmingAMD = 0;
#endif
    PFN_vkTrimCommandPool vkTrimCommandPool = 0;
    PFN_vkTrimCommandPoolKHR vkTrimCommandPoolKHR = 0;
    PFN_vkUnmapMemory vkUnmapMemory = 0;
    PFN_vkUnregisterObjectsNVX vkUnregisterObjectsNVX = 0;
    PFN_vkUpdateDescriptorSetWithTemplate vkUpdateDescriptorSetWithTemplate = 0;
    PFN_vkUpdateDescriptorSetWithTemplateKHR vkUpdateDescriptorSetWithTemplateKHR = 0;
    PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets = 0;
    PFN_vkWaitForFences vkWaitForFences = 0;
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurfaceKHR = 0;
#endif /*VK_USE_PLATFORM_ANDROID_KHR*/
    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = 0;
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = 0;
    PFN_vkCreateDisplayPlaneSurfaceKHR vkCreateDisplayPlaneSurfaceKHR = 0;
#if defined(VK_EXT_headless_surface)
    PFN_vkCreateHeadlessSurfaceEXT vkCreateHeadlessSurfaceEXT = 0;
#endif
#ifdef VK_USE_PLATFORM_IOS_MVK
    PFN_vkCreateIOSSurfaceMVK vkCreateIOSSurfaceMVK = 0;
#endif /*VK_USE_PLATFORM_IOS_MVK*/
#ifdef VK_USE_PLATFORM_FUCHSIA
    PFN_vkCreateImagePipeSurfaceFUCHSIA vkCreateImagePipeSurfaceFUCHSIA = 0;
#endif /*VK_USE_PLATFORM_FUCHSIA*/
#ifdef VK_USE_PLATFORM_MACOS_MVK
    PFN_vkCreateMacOSSurfaceMVK vkCreateMacOSSurfaceMVK = 0;
#endif /*VK_USE_PLATFORM_MACOS_MVK*/
#ifdef VK_USE_PLATFORM_METAL_EXT
    PFN_vkCreateMetalSurfaceEXT vkCreateMetalSurfaceEXT = 0;
#endif /*VK_USE_PLATFORM_METAL_EXT*/
#ifdef VK_USE_PLATFORM_GGP
    PFN_vkCreateStreamDescriptorSurfaceGGP vkCreateStreamDescriptorSurfaceGGP = 0;
#endif /*VK_USE_PLATFORM_GGP*/
#ifdef VK_USE_PLATFORM_VI_NN
    PFN_vkCreateViSurfaceNN vkCreateViSurfaceNN = 0;
#endif /*VK_USE_PLATFORM_VI_NN*/
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR = 0;
#endif /*VK_USE_PLATFORM_WAYLAND_KHR*/
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = 0;
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
#ifdef VK_USE_PLATFORM_XCB_KHR
    PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR = 0;
#endif /*VK_USE_PLATFORM_XCB_KHR*/
#ifdef VK_USE_PLATFORM_XLIB_KHR
    PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR = 0;
#endif /*VK_USE_PLATFORM_XLIB_KHR*/
    PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT = 0;
    PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = 0;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = 0;
    PFN_vkDestroyInstance vkDestroyInstance = 0;
    PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR = 0;
    PFN_vkEnumeratePhysicalDeviceGroups vkEnumeratePhysicalDeviceGroups = 0;
    PFN_vkEnumeratePhysicalDeviceGroupsKHR vkEnumeratePhysicalDeviceGroupsKHR = 0;
    PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = 0;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = 0;
    PFN_vkSubmitDebugUtilsMessageEXT vkSubmitDebugUtilsMessageEXT = 0;
#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
    PFN_vkAcquireXlibDisplayEXT vkAcquireXlibDisplayEXT = 0;
#endif /*VK_USE_PLATFORM_XLIB_XRANDR_EXT*/
    PFN_vkCreateDevice vkCreateDevice = 0;
    PFN_vkCreateDisplayModeKHR vkCreateDisplayModeKHR = 0;
    PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties = 0;
    PFN_vkEnumerateDeviceLayerProperties vkEnumerateDeviceLayerProperties = 0;
    PFN_vkGetDisplayModeProperties2KHR vkGetDisplayModeProperties2KHR = 0;
    PFN_vkGetDisplayModePropertiesKHR vkGetDisplayModePropertiesKHR = 0;
    PFN_vkGetDisplayPlaneCapabilities2KHR vkGetDisplayPlaneCapabilities2KHR = 0;
    PFN_vkGetDisplayPlaneCapabilitiesKHR vkGetDisplayPlaneCapabilitiesKHR = 0;
    PFN_vkGetDisplayPlaneSupportedDisplaysKHR vkGetDisplayPlaneSupportedDisplaysKHR = 0;
    PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT vkGetPhysicalDeviceCalibrateableTimeDomainsEXT = 0;
#if defined(VK_NV_cooperative_matrix)
    PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV vkGetPhysicalDeviceCooperativeMatrixPropertiesNV = 0;
#endif
    PFN_vkGetPhysicalDeviceDisplayPlaneProperties2KHR vkGetPhysicalDeviceDisplayPlaneProperties2KHR = 0;
    PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR vkGetPhysicalDeviceDisplayPlanePropertiesKHR = 0;
    PFN_vkGetPhysicalDeviceDisplayProperties2KHR vkGetPhysicalDeviceDisplayProperties2KHR = 0;
    PFN_vkGetPhysicalDeviceDisplayPropertiesKHR vkGetPhysicalDeviceDisplayPropertiesKHR = 0;
    PFN_vkGetPhysicalDeviceExternalBufferProperties vkGetPhysicalDeviceExternalBufferProperties = 0;
    PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR vkGetPhysicalDeviceExternalBufferPropertiesKHR = 0;
    PFN_vkGetPhysicalDeviceExternalFenceProperties vkGetPhysicalDeviceExternalFenceProperties = 0;
    PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR vkGetPhysicalDeviceExternalFencePropertiesKHR = 0;
    PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV vkGetPhysicalDeviceExternalImageFormatPropertiesNV = 0;
    PFN_vkGetPhysicalDeviceExternalSemaphoreProperties vkGetPhysicalDeviceExternalSemaphoreProperties = 0;
    PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR vkGetPhysicalDeviceExternalSemaphorePropertiesKHR = 0;
    PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures = 0;
    PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2 = 0;
    PFN_vkGetPhysicalDeviceFeatures2KHR vkGetPhysicalDeviceFeatures2KHR = 0;
    PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties = 0;
    PFN_vkGetPhysicalDeviceFormatProperties2 vkGetPhysicalDeviceFormatProperties2 = 0;
    PFN_vkGetPhysicalDeviceFormatProperties2KHR vkGetPhysicalDeviceFormatProperties2KHR = 0;
    PFN_vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX = 0;
    PFN_vkGetPhysicalDeviceImageFormatProperties vkGetPhysicalDeviceImageFormatProperties = 0;
    PFN_vkGetPhysicalDeviceImageFormatProperties2 vkGetPhysicalDeviceImageFormatProperties2 = 0;
    PFN_vkGetPhysicalDeviceImageFormatProperties2KHR vkGetPhysicalDeviceImageFormatProperties2KHR = 0;
    PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties = 0;
    PFN_vkGetPhysicalDeviceMemoryProperties2 vkGetPhysicalDeviceMemoryProperties2 = 0;
    PFN_vkGetPhysicalDeviceMemoryProperties2KHR vkGetPhysicalDeviceMemoryProperties2KHR = 0;
    PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT vkGetPhysicalDeviceMultisamplePropertiesEXT = 0;
    PFN_vkGetPhysicalDevicePresentRectanglesKHR vkGetPhysicalDevicePresentRectanglesKHR = 0;
    PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = 0;
    PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2 = 0;
    PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR = 0;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties = 0;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties2 vkGetPhysicalDeviceQueueFamilyProperties2 = 0;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR vkGetPhysicalDeviceQueueFamilyProperties2KHR = 0;
    PFN_vkGetPhysicalDeviceSparseImageFormatProperties vkGetPhysicalDeviceSparseImageFormatProperties = 0;
    PFN_vkGetPhysicalDeviceSparseImageFormatProperties2 vkGetPhysicalDeviceSparseImageFormatProperties2 = 0;
    PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR vkGetPhysicalDeviceSparseImageFormatProperties2KHR = 0;
    PFN_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV = 0;
    PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT vkGetPhysicalDeviceSurfaceCapabilities2EXT = 0;
    PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR vkGetPhysicalDeviceSurfaceCapabilities2KHR = 0;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR = 0;
    PFN_vkGetPhysicalDeviceSurfaceFormats2KHR vkGetPhysicalDeviceSurfaceFormats2KHR = 0;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR = 0;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkGetPhysicalDeviceSurfacePresentModes2EXT vkGetPhysicalDeviceSurfacePresentModes2EXT = 0;
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR = 0;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR = 0;
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR vkGetPhysicalDeviceWaylandPresentationSupportKHR = 0;
#endif /*VK_USE_PLATFORM_WAYLAND_KHR*/
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR vkGetPhysicalDeviceWin32PresentationSupportKHR = 0;
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
#ifdef VK_USE_PLATFORM_XCB_KHR
    PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR vkGetPhysicalDeviceXcbPresentationSupportKHR = 0;
#endif /*VK_USE_PLATFORM_XCB_KHR*/
#ifdef VK_USE_PLATFORM_XLIB_KHR
    PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR vkGetPhysicalDeviceXlibPresentationSupportKHR = 0;
#endif /*VK_USE_PLATFORM_XLIB_KHR*/
#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
    PFN_vkGetRandROutputDisplayEXT vkGetRandROutputDisplayEXT = 0;
#endif /*VK_USE_PLATFORM_XLIB_XRANDR_EXT*/
    PFN_vkReleaseDisplayEXT vkReleaseDisplayEXT = 0;
    PFN_vkGetQueueCheckpointDataNV vkGetQueueCheckpointDataNV = 0;
    PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginDebugUtilsLabelEXT = 0;
    PFN_vkQueueBindSparse vkQueueBindSparse = 0;
    PFN_vkQueueEndDebugUtilsLabelEXT vkQueueEndDebugUtilsLabelEXT = 0;
    PFN_vkQueueInsertDebugUtilsLabelEXT vkQueueInsertDebugUtilsLabelEXT = 0;
    PFN_vkQueuePresentKHR vkQueuePresentKHR = 0;
    PFN_vkQueueSubmit vkQueueSubmit = 0;
    PFN_vkQueueWaitIdle vkQueueWaitIdle = 0;

  public:
    DispatchLoaderDynamic() = default;

//    // This interface is designed to be used for per-device function pointers in combination with a linked vulkan library.
//    DispatchLoaderDynamic(vk::Instance const& instance, vk::Device const& device = {})
//    {
//      init(instance, device);
//    }
//
//    // This interface is designed to be used for per-device function pointers in combination with a linked vulkan library.
//    void init(vk::Instance const& instance, vk::Device const& device = {})
//    {
//      init(instance, ::vkGetInstanceProcAddr, device, device ? ::vkGetDeviceProcAddr : nullptr);
//    }

    // This interface does not require a linked vulkan library.
    DispatchLoaderDynamic( VkInstance instance, PFN_vkGetInstanceProcAddr getInstanceProcAddr, VkDevice device = VK_NULL_HANDLE, PFN_vkGetDeviceProcAddr getDeviceProcAddr = nullptr )
    {
      init( instance, getInstanceProcAddr, device, getDeviceProcAddr );
    }

    // This interface does not require a linked vulkan library.
    void init( VkInstance instance, PFN_vkGetInstanceProcAddr getInstanceProcAddr, VkDevice device = VK_NULL_HANDLE, PFN_vkGetDeviceProcAddr getDeviceProcAddr = nullptr )
    {
      assert(instance && getInstanceProcAddr);
      assert(!!device == !!getDeviceProcAddr);
      vkGetInstanceProcAddr = getInstanceProcAddr;
      vkGetDeviceProcAddr = getDeviceProcAddr ? getDeviceProcAddr : PFN_vkGetDeviceProcAddr( vkGetInstanceProcAddr( instance, "vkGetDeviceProcAddr") );
      vkCreateInstance = PFN_vkCreateInstance( vkGetInstanceProcAddr( instance, "vkCreateInstance" ) );
      vkEnumerateInstanceExtensionProperties = PFN_vkEnumerateInstanceExtensionProperties( vkGetInstanceProcAddr( instance, "vkEnumerateInstanceExtensionProperties" ) );
      vkEnumerateInstanceLayerProperties = PFN_vkEnumerateInstanceLayerProperties( vkGetInstanceProcAddr( instance, "vkEnumerateInstanceLayerProperties" ) );
      vkEnumerateInstanceVersion = PFN_vkEnumerateInstanceVersion( vkGetInstanceProcAddr( instance, "vkEnumerateInstanceVersion" ) );
      vkBeginCommandBuffer = PFN_vkBeginCommandBuffer( device ? vkGetDeviceProcAddr( device, "vkBeginCommandBuffer" ) : vkGetInstanceProcAddr( instance, "vkBeginCommandBuffer" ) );
      vkCmdBeginConditionalRenderingEXT = PFN_vkCmdBeginConditionalRenderingEXT( device ? vkGetDeviceProcAddr( device, "vkCmdBeginConditionalRenderingEXT" ) : vkGetInstanceProcAddr( instance, "vkCmdBeginConditionalRenderingEXT" ) );
      vkCmdBeginDebugUtilsLabelEXT = PFN_vkCmdBeginDebugUtilsLabelEXT( device ? vkGetDeviceProcAddr( device, "vkCmdBeginDebugUtilsLabelEXT" ) : vkGetInstanceProcAddr( instance, "vkCmdBeginDebugUtilsLabelEXT" ) );
      vkCmdBeginQuery = PFN_vkCmdBeginQuery( device ? vkGetDeviceProcAddr( device, "vkCmdBeginQuery" ) : vkGetInstanceProcAddr( instance, "vkCmdBeginQuery" ) );
#if defined(VK_EXT_transform_feedback)
      vkCmdBeginQueryIndexedEXT = PFN_vkCmdBeginQueryIndexedEXT( device ? vkGetDeviceProcAddr( device, "vkCmdBeginQueryIndexedEXT" ) : vkGetInstanceProcAddr( instance, "vkCmdBeginQueryIndexedEXT" ) );
#endif
      vkCmdBeginRenderPass = PFN_vkCmdBeginRenderPass( device ? vkGetDeviceProcAddr( device, "vkCmdBeginRenderPass" ) : vkGetInstanceProcAddr( instance, "vkCmdBeginRenderPass" ) );
      vkCmdBeginRenderPass2KHR = PFN_vkCmdBeginRenderPass2KHR( device ? vkGetDeviceProcAddr( device, "vkCmdBeginRenderPass2KHR" ) : vkGetInstanceProcAddr( instance, "vkCmdBeginRenderPass2KHR" ) );
#if defined(VK_EXT_transform_feedback)
      vkCmdBeginTransformFeedbackEXT = PFN_vkCmdBeginTransformFeedbackEXT( device ? vkGetDeviceProcAddr( device, "vkCmdBeginTransformFeedbackEXT" ) : vkGetInstanceProcAddr( instance, "vkCmdBeginTransformFeedbackEXT" ) );
#endif
      vkCmdBindDescriptorSets = PFN_vkCmdBindDescriptorSets( device ? vkGetDeviceProcAddr( device, "vkCmdBindDescriptorSets" ) : vkGetInstanceProcAddr( instance, "vkCmdBindDescriptorSets" ) );
      vkCmdBindIndexBuffer = PFN_vkCmdBindIndexBuffer( device ? vkGetDeviceProcAddr( device, "vkCmdBindIndexBuffer" ) : vkGetInstanceProcAddr( instance, "vkCmdBindIndexBuffer" ) );
      vkCmdBindPipeline = PFN_vkCmdBindPipeline( device ? vkGetDeviceProcAddr( device, "vkCmdBindPipeline" ) : vkGetInstanceProcAddr( instance, "vkCmdBindPipeline" ) );
#if defined(VK_NV_shading_rate_image)
      vkCmdBindShadingRateImageNV = PFN_vkCmdBindShadingRateImageNV( device ? vkGetDeviceProcAddr( device, "vkCmdBindShadingRateImageNV" ) : vkGetInstanceProcAddr( instance, "vkCmdBindShadingRateImageNV" ) );
#endif
#if defined(VK_EXT_transform_feedback)
      vkCmdBindTransformFeedbackBuffersEXT = PFN_vkCmdBindTransformFeedbackBuffersEXT( device ? vkGetDeviceProcAddr( device, "vkCmdBindTransformFeedbackBuffersEXT" ) : vkGetInstanceProcAddr( instance, "vkCmdBindTransformFeedbackBuffersEXT" ) );
#endif
      vkCmdBindVertexBuffers = PFN_vkCmdBindVertexBuffers( device ? vkGetDeviceProcAddr( device, "vkCmdBindVertexBuffers" ) : vkGetInstanceProcAddr( instance, "vkCmdBindVertexBuffers" ) );
      vkCmdBlitImage = PFN_vkCmdBlitImage( device ? vkGetDeviceProcAddr( device, "vkCmdBlitImage" ) : vkGetInstanceProcAddr( instance, "vkCmdBlitImage" ) );
#if defined(VK_NV_ray_tracing)
      vkCmdBuildAccelerationStructureNV = PFN_vkCmdBuildAccelerationStructureNV( device ? vkGetDeviceProcAddr( device, "vkCmdBuildAccelerationStructureNV" ) : vkGetInstanceProcAddr( instance, "vkCmdBuildAccelerationStructureNV" ) );
#endif
      vkCmdClearAttachments = PFN_vkCmdClearAttachments( device ? vkGetDeviceProcAddr( device, "vkCmdClearAttachments" ) : vkGetInstanceProcAddr( instance, "vkCmdClearAttachments" ) );
      vkCmdClearColorImage = PFN_vkCmdClearColorImage( device ? vkGetDeviceProcAddr( device, "vkCmdClearColorImage" ) : vkGetInstanceProcAddr( instance, "vkCmdClearColorImage" ) );
      vkCmdClearDepthStencilImage = PFN_vkCmdClearDepthStencilImage( device ? vkGetDeviceProcAddr( device, "vkCmdClearDepthStencilImage" ) : vkGetInstanceProcAddr( instance, "vkCmdClearDepthStencilImage" ) );
#if defined(VK_NV_ray_tracing)
      vkCmdCopyAccelerationStructureNV = PFN_vkCmdCopyAccelerationStructureNV( device ? vkGetDeviceProcAddr( device, "vkCmdCopyAccelerationStructureNV" ) : vkGetInstanceProcAddr( instance, "vkCmdCopyAccelerationStructureNV" ) );
#endif
      vkCmdCopyBuffer = PFN_vkCmdCopyBuffer( device ? vkGetDeviceProcAddr( device, "vkCmdCopyBuffer" ) : vkGetInstanceProcAddr( instance, "vkCmdCopyBuffer" ) );
      vkCmdCopyBufferToImage = PFN_vkCmdCopyBufferToImage( device ? vkGetDeviceProcAddr( device, "vkCmdCopyBufferToImage" ) : vkGetInstanceProcAddr( instance, "vkCmdCopyBufferToImage" ) );
      vkCmdCopyImage = PFN_vkCmdCopyImage( device ? vkGetDeviceProcAddr( device, "vkCmdCopyImage" ) : vkGetInstanceProcAddr( instance, "vkCmdCopyImage" ) );
      vkCmdCopyImageToBuffer = PFN_vkCmdCopyImageToBuffer( device ? vkGetDeviceProcAddr( device, "vkCmdCopyImageToBuffer" ) : vkGetInstanceProcAddr( instance, "vkCmdCopyImageToBuffer" ) );
      vkCmdCopyQueryPoolResults = PFN_vkCmdCopyQueryPoolResults( device ? vkGetDeviceProcAddr( device, "vkCmdCopyQueryPoolResults" ) : vkGetInstanceProcAddr( instance, "vkCmdCopyQueryPoolResults" ) );
      vkCmdDebugMarkerBeginEXT = PFN_vkCmdDebugMarkerBeginEXT( device ? vkGetDeviceProcAddr( device, "vkCmdDebugMarkerBeginEXT" ) : vkGetInstanceProcAddr( instance, "vkCmdDebugMarkerBeginEXT" ) );
      vkCmdDebugMarkerEndEXT = PFN_vkCmdDebugMarkerEndEXT( device ? vkGetDeviceProcAddr( device, "vkCmdDebugMarkerEndEXT" ) : vkGetInstanceProcAddr( instance, "vkCmdDebugMarkerEndEXT" ) );
      vkCmdDebugMarkerInsertEXT = PFN_vkCmdDebugMarkerInsertEXT( device ? vkGetDeviceProcAddr( device, "vkCmdDebugMarkerInsertEXT" ) : vkGetInstanceProcAddr( instance, "vkCmdDebugMarkerInsertEXT" ) );
      vkCmdDispatch = PFN_vkCmdDispatch( device ? vkGetDeviceProcAddr( device, "vkCmdDispatch" ) : vkGetInstanceProcAddr( instance, "vkCmdDispatch" ) );
      vkCmdDispatchBase = PFN_vkCmdDispatchBase( device ? vkGetDeviceProcAddr( device, "vkCmdDispatchBase" ) : vkGetInstanceProcAddr( instance, "vkCmdDispatchBase" ) );
      vkCmdDispatchBaseKHR = PFN_vkCmdDispatchBaseKHR( device ? vkGetDeviceProcAddr( device, "vkCmdDispatchBaseKHR" ) : vkGetInstanceProcAddr( instance, "vkCmdDispatchBaseKHR" ) );
      vkCmdDispatchIndirect = PFN_vkCmdDispatchIndirect( device ? vkGetDeviceProcAddr( device, "vkCmdDispatchIndirect" ) : vkGetInstanceProcAddr( instance, "vkCmdDispatchIndirect" ) );
      vkCmdDraw = PFN_vkCmdDraw( device ? vkGetDeviceProcAddr( device, "vkCmdDraw" ) : vkGetInstanceProcAddr( instance, "vkCmdDraw" ) );
      vkCmdDrawIndexed = PFN_vkCmdDrawIndexed( device ? vkGetDeviceProcAddr( device, "vkCmdDrawIndexed" ) : vkGetInstanceProcAddr( instance, "vkCmdDrawIndexed" ) );
      vkCmdDrawIndexedIndirect = PFN_vkCmdDrawIndexedIndirect( device ? vkGetDeviceProcAddr( device, "vkCmdDrawIndexedIndirect" ) : vkGetInstanceProcAddr( instance, "vkCmdDrawIndexedIndirect" ) );
      vkCmdDrawIndexedIndirectCountAMD = PFN_vkCmdDrawIndexedIndirectCountAMD( device ? vkGetDeviceProcAddr( device, "vkCmdDrawIndexedIndirectCountAMD" ) : vkGetInstanceProcAddr( instance, "vkCmdDrawIndexedIndirectCountAMD" ) );
#if defined(VK_KHR_draw_indirect_count)
      vkCmdDrawIndexedIndirectCountKHR = PFN_vkCmdDrawIndexedIndirectCountKHR( device ? vkGetDeviceProcAddr( device, "vkCmdDrawIndexedIndirectCountKHR" ) : vkGetInstanceProcAddr( instance, "vkCmdDrawIndexedIndirectCountKHR" ) );
#endif
      vkCmdDrawIndirect = PFN_vkCmdDrawIndirect( device ? vkGetDeviceProcAddr( device, "vkCmdDrawIndirect" ) : vkGetInstanceProcAddr( instance, "vkCmdDrawIndirect" ) );
#if defined(VK_EXT_transform_feedback)
      vkCmdDrawIndirectByteCountEXT = PFN_vkCmdDrawIndirectByteCountEXT( device ? vkGetDeviceProcAddr( device, "vkCmdDrawIndirectByteCountEXT" ) : vkGetInstanceProcAddr( instance, "vkCmdDrawIndirectByteCountEXT" ) );
#endif
      vkCmdDrawIndirectCountAMD = PFN_vkCmdDrawIndirectCountAMD( device ? vkGetDeviceProcAddr( device, "vkCmdDrawIndirectCountAMD" ) : vkGetInstanceProcAddr( instance, "vkCmdDrawIndirectCountAMD" ) );
#if defined(VK_KHR_draw_indirect_count)
      vkCmdDrawIndirectCountKHR = PFN_vkCmdDrawIndirectCountKHR( device ? vkGetDeviceProcAddr( device, "vkCmdDrawIndirectCountKHR" ) : vkGetInstanceProcAddr( instance, "vkCmdDrawIndirectCountKHR" ) );
#endif
#if defined(VK_NV_mesh_shader)
      vkCmdDrawMeshTasksIndirectCountNV = PFN_vkCmdDrawMeshTasksIndirectCountNV( device ? vkGetDeviceProcAddr( device, "vkCmdDrawMeshTasksIndirectCountNV" ) : vkGetInstanceProcAddr( instance, "vkCmdDrawMeshTasksIndirectCountNV" ) );
      vkCmdDrawMeshTasksIndirectNV = PFN_vkCmdDrawMeshTasksIndirectNV( device ? vkGetDeviceProcAddr( device, "vkCmdDrawMeshTasksIndirectNV" ) : vkGetInstanceProcAddr( instance, "vkCmdDrawMeshTasksIndirectNV" ) );
      vkCmdDrawMeshTasksNV = PFN_vkCmdDrawMeshTasksNV( device ? vkGetDeviceProcAddr( device, "vkCmdDrawMeshTasksNV" ) : vkGetInstanceProcAddr( instance, "vkCmdDrawMeshTasksNV" ) );
#endif
      vkCmdEndConditionalRenderingEXT = PFN_vkCmdEndConditionalRenderingEXT( device ? vkGetDeviceProcAddr( device, "vkCmdEndConditionalRenderingEXT" ) : vkGetInstanceProcAddr( instance, "vkCmdEndConditionalRenderingEXT" ) );
      vkCmdEndDebugUtilsLabelEXT = PFN_vkCmdEndDebugUtilsLabelEXT( device ? vkGetDeviceProcAddr( device, "vkCmdEndDebugUtilsLabelEXT" ) : vkGetInstanceProcAddr( instance, "vkCmdEndDebugUtilsLabelEXT" ) );
      vkCmdEndQuery = PFN_vkCmdEndQuery( device ? vkGetDeviceProcAddr( device, "vkCmdEndQuery" ) : vkGetInstanceProcAddr( instance, "vkCmdEndQuery" ) );
#if defined(VK_EXT_transform_feedback)
      vkCmdEndQueryIndexedEXT = PFN_vkCmdEndQueryIndexedEXT( device ? vkGetDeviceProcAddr( device, "vkCmdEndQueryIndexedEXT" ) : vkGetInstanceProcAddr( instance, "vkCmdEndQueryIndexedEXT" ) );
#endif
      vkCmdEndRenderPass = PFN_vkCmdEndRenderPass( device ? vkGetDeviceProcAddr( device, "vkCmdEndRenderPass" ) : vkGetInstanceProcAddr( instance, "vkCmdEndRenderPass" ) );
      vkCmdEndRenderPass2KHR = PFN_vkCmdEndRenderPass2KHR( device ? vkGetDeviceProcAddr( device, "vkCmdEndRenderPass2KHR" ) : vkGetInstanceProcAddr( instance, "vkCmdEndRenderPass2KHR" ) );
#if defined(VK_EXT_transform_feedback)
      vkCmdEndTransformFeedbackEXT = PFN_vkCmdEndTransformFeedbackEXT( device ? vkGetDeviceProcAddr( device, "vkCmdEndTransformFeedbackEXT" ) : vkGetInstanceProcAddr( instance, "vkCmdEndTransformFeedbackEXT" ) );
#endif
      vkCmdExecuteCommands = PFN_vkCmdExecuteCommands( device ? vkGetDeviceProcAddr( device, "vkCmdExecuteCommands" ) : vkGetInstanceProcAddr( instance, "vkCmdExecuteCommands" ) );
      vkCmdFillBuffer = PFN_vkCmdFillBuffer( device ? vkGetDeviceProcAddr( device, "vkCmdFillBuffer" ) : vkGetInstanceProcAddr( instance, "vkCmdFillBuffer" ) );
      vkCmdInsertDebugUtilsLabelEXT = PFN_vkCmdInsertDebugUtilsLabelEXT( device ? vkGetDeviceProcAddr( device, "vkCmdInsertDebugUtilsLabelEXT" ) : vkGetInstanceProcAddr( instance, "vkCmdInsertDebugUtilsLabelEXT" ) );
      vkCmdNextSubpass = PFN_vkCmdNextSubpass( device ? vkGetDeviceProcAddr( device, "vkCmdNextSubpass" ) : vkGetInstanceProcAddr( instance, "vkCmdNextSubpass" ) );
      vkCmdNextSubpass2KHR = PFN_vkCmdNextSubpass2KHR( device ? vkGetDeviceProcAddr( device, "vkCmdNextSubpass2KHR" ) : vkGetInstanceProcAddr( instance, "vkCmdNextSubpass2KHR" ) );
      vkCmdPipelineBarrier = PFN_vkCmdPipelineBarrier( device ? vkGetDeviceProcAddr( device, "vkCmdPipelineBarrier" ) : vkGetInstanceProcAddr( instance, "vkCmdPipelineBarrier" ) );
      vkCmdProcessCommandsNVX = PFN_vkCmdProcessCommandsNVX( device ? vkGetDeviceProcAddr( device, "vkCmdProcessCommandsNVX" ) : vkGetInstanceProcAddr( instance, "vkCmdProcessCommandsNVX" ) );
      vkCmdPushConstants = PFN_vkCmdPushConstants( device ? vkGetDeviceProcAddr( device, "vkCmdPushConstants" ) : vkGetInstanceProcAddr( instance, "vkCmdPushConstants" ) );
      vkCmdPushDescriptorSetKHR = PFN_vkCmdPushDescriptorSetKHR( device ? vkGetDeviceProcAddr( device, "vkCmdPushDescriptorSetKHR" ) : vkGetInstanceProcAddr( instance, "vkCmdPushDescriptorSetKHR" ) );
      vkCmdPushDescriptorSetWithTemplateKHR = PFN_vkCmdPushDescriptorSetWithTemplateKHR( device ? vkGetDeviceProcAddr( device, "vkCmdPushDescriptorSetWithTemplateKHR" ) : vkGetInstanceProcAddr( instance, "vkCmdPushDescriptorSetWithTemplateKHR" ) );
      vkCmdReserveSpaceForCommandsNVX = PFN_vkCmdReserveSpaceForCommandsNVX( device ? vkGetDeviceProcAddr( device, "vkCmdReserveSpaceForCommandsNVX" ) : vkGetInstanceProcAddr( instance, "vkCmdReserveSpaceForCommandsNVX" ) );
      vkCmdResetEvent = PFN_vkCmdResetEvent( device ? vkGetDeviceProcAddr( device, "vkCmdResetEvent" ) : vkGetInstanceProcAddr( instance, "vkCmdResetEvent" ) );
      vkCmdResetQueryPool = PFN_vkCmdResetQueryPool( device ? vkGetDeviceProcAddr( device, "vkCmdResetQueryPool" ) : vkGetInstanceProcAddr( instance, "vkCmdResetQueryPool" ) );
      vkCmdResolveImage = PFN_vkCmdResolveImage( device ? vkGetDeviceProcAddr( device, "vkCmdResolveImage" ) : vkGetInstanceProcAddr( instance, "vkCmdResolveImage" ) );
      vkCmdSetBlendConstants = PFN_vkCmdSetBlendConstants( device ? vkGetDeviceProcAddr( device, "vkCmdSetBlendConstants" ) : vkGetInstanceProcAddr( instance, "vkCmdSetBlendConstants" ) );
      vkCmdSetCheckpointNV = PFN_vkCmdSetCheckpointNV( device ? vkGetDeviceProcAddr( device, "vkCmdSetCheckpointNV" ) : vkGetInstanceProcAddr( instance, "vkCmdSetCheckpointNV" ) );
#if defined(VK_NV_shading_rate_image)
      vkCmdSetCoarseSampleOrderNV = PFN_vkCmdSetCoarseSampleOrderNV( device ? vkGetDeviceProcAddr( device, "vkCmdSetCoarseSampleOrderNV" ) : vkGetInstanceProcAddr( instance, "vkCmdSetCoarseSampleOrderNV" ) );
#endif
      vkCmdSetDepthBias = PFN_vkCmdSetDepthBias( device ? vkGetDeviceProcAddr( device, "vkCmdSetDepthBias" ) : vkGetInstanceProcAddr( instance, "vkCmdSetDepthBias" ) );
      vkCmdSetDepthBounds = PFN_vkCmdSetDepthBounds( device ? vkGetDeviceProcAddr( device, "vkCmdSetDepthBounds" ) : vkGetInstanceProcAddr( instance, "vkCmdSetDepthBounds" ) );
      vkCmdSetDeviceMask = PFN_vkCmdSetDeviceMask( device ? vkGetDeviceProcAddr( device, "vkCmdSetDeviceMask" ) : vkGetInstanceProcAddr( instance, "vkCmdSetDeviceMask" ) );
      vkCmdSetDeviceMaskKHR = PFN_vkCmdSetDeviceMaskKHR( device ? vkGetDeviceProcAddr( device, "vkCmdSetDeviceMaskKHR" ) : vkGetInstanceProcAddr( instance, "vkCmdSetDeviceMaskKHR" ) );
      vkCmdSetDiscardRectangleEXT = PFN_vkCmdSetDiscardRectangleEXT( device ? vkGetDeviceProcAddr( device, "vkCmdSetDiscardRectangleEXT" ) : vkGetInstanceProcAddr( instance, "vkCmdSetDiscardRectangleEXT" ) );
      vkCmdSetEvent = PFN_vkCmdSetEvent( device ? vkGetDeviceProcAddr( device, "vkCmdSetEvent" ) : vkGetInstanceProcAddr( instance, "vkCmdSetEvent" ) );
#if defined(VK_NV_scissor_exclusive)
      vkCmdSetExclusiveScissorNV = PFN_vkCmdSetExclusiveScissorNV( device ? vkGetDeviceProcAddr( device, "vkCmdSetExclusiveScissorNV" ) : vkGetInstanceProcAddr( instance, "vkCmdSetExclusiveScissorNV" ) );
#endif
      vkCmdSetLineWidth = PFN_vkCmdSetLineWidth( device ? vkGetDeviceProcAddr( device, "vkCmdSetLineWidth" ) : vkGetInstanceProcAddr( instance, "vkCmdSetLineWidth" ) );
      vkCmdSetSampleLocationsEXT = PFN_vkCmdSetSampleLocationsEXT( device ? vkGetDeviceProcAddr( device, "vkCmdSetSampleLocationsEXT" ) : vkGetInstanceProcAddr( instance, "vkCmdSetSampleLocationsEXT" ) );
      vkCmdSetScissor = PFN_vkCmdSetScissor( device ? vkGetDeviceProcAddr( device, "vkCmdSetScissor" ) : vkGetInstanceProcAddr( instance, "vkCmdSetScissor" ) );
      vkCmdSetStencilCompareMask = PFN_vkCmdSetStencilCompareMask( device ? vkGetDeviceProcAddr( device, "vkCmdSetStencilCompareMask" ) : vkGetInstanceProcAddr( instance, "vkCmdSetStencilCompareMask" ) );
      vkCmdSetStencilReference = PFN_vkCmdSetStencilReference( device ? vkGetDeviceProcAddr( device, "vkCmdSetStencilReference" ) : vkGetInstanceProcAddr( instance, "vkCmdSetStencilReference" ) );
      vkCmdSetStencilWriteMask = PFN_vkCmdSetStencilWriteMask( device ? vkGetDeviceProcAddr( device, "vkCmdSetStencilWriteMask" ) : vkGetInstanceProcAddr( instance, "vkCmdSetStencilWriteMask" ) );
      vkCmdSetViewport = PFN_vkCmdSetViewport( device ? vkGetDeviceProcAddr( device, "vkCmdSetViewport" ) : vkGetInstanceProcAddr( instance, "vkCmdSetViewport" ) );
#if defined(VK_NV_shading_rate_image)
      vkCmdSetViewportShadingRatePaletteNV = PFN_vkCmdSetViewportShadingRatePaletteNV( device ? vkGetDeviceProcAddr( device, "vkCmdSetViewportShadingRatePaletteNV" ) : vkGetInstanceProcAddr( instance, "vkCmdSetViewportShadingRatePaletteNV" ) );
#endif
      vkCmdSetViewportWScalingNV = PFN_vkCmdSetViewportWScalingNV( device ? vkGetDeviceProcAddr( device, "vkCmdSetViewportWScalingNV" ) : vkGetInstanceProcAddr( instance, "vkCmdSetViewportWScalingNV" ) );
#if defined(VK_NV_ray_tracing)
      vkCmdTraceRaysNV = PFN_vkCmdTraceRaysNV( device ? vkGetDeviceProcAddr( device, "vkCmdTraceRaysNV" ) : vkGetInstanceProcAddr( instance, "vkCmdTraceRaysNV" ) );
#endif
      vkCmdUpdateBuffer = PFN_vkCmdUpdateBuffer( device ? vkGetDeviceProcAddr( device, "vkCmdUpdateBuffer" ) : vkGetInstanceProcAddr( instance, "vkCmdUpdateBuffer" ) );
      vkCmdWaitEvents = PFN_vkCmdWaitEvents( device ? vkGetDeviceProcAddr( device, "vkCmdWaitEvents" ) : vkGetInstanceProcAddr( instance, "vkCmdWaitEvents" ) );
#if defined(VK_NV_ray_tracing)
      vkCmdWriteAccelerationStructuresPropertiesNV = PFN_vkCmdWriteAccelerationStructuresPropertiesNV( device ? vkGetDeviceProcAddr( device, "vkCmdWriteAccelerationStructuresPropertiesNV" ) : vkGetInstanceProcAddr( instance, "vkCmdWriteAccelerationStructuresPropertiesNV" ) );
#endif
      vkCmdWriteBufferMarkerAMD = PFN_vkCmdWriteBufferMarkerAMD( device ? vkGetDeviceProcAddr( device, "vkCmdWriteBufferMarkerAMD" ) : vkGetInstanceProcAddr( instance, "vkCmdWriteBufferMarkerAMD" ) );
      vkCmdWriteTimestamp = PFN_vkCmdWriteTimestamp( device ? vkGetDeviceProcAddr( device, "vkCmdWriteTimestamp" ) : vkGetInstanceProcAddr( instance, "vkCmdWriteTimestamp" ) );
      vkEndCommandBuffer = PFN_vkEndCommandBuffer( device ? vkGetDeviceProcAddr( device, "vkEndCommandBuffer" ) : vkGetInstanceProcAddr( instance, "vkEndCommandBuffer" ) );
      vkResetCommandBuffer = PFN_vkResetCommandBuffer( device ? vkGetDeviceProcAddr( device, "vkResetCommandBuffer" ) : vkGetInstanceProcAddr( instance, "vkResetCommandBuffer" ) );
#ifdef VK_USE_PLATFORM_WIN32_KHR
      vkAcquireFullScreenExclusiveModeEXT = PFN_vkAcquireFullScreenExclusiveModeEXT( device ? vkGetDeviceProcAddr( device, "vkAcquireFullScreenExclusiveModeEXT" ) : vkGetInstanceProcAddr( instance, "vkAcquireFullScreenExclusiveModeEXT" ) );
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
      vkAcquireNextImage2KHR = PFN_vkAcquireNextImage2KHR( device ? vkGetDeviceProcAddr( device, "vkAcquireNextImage2KHR" ) : vkGetInstanceProcAddr( instance, "vkAcquireNextImage2KHR" ) );
      vkAcquireNextImageKHR = PFN_vkAcquireNextImageKHR( device ? vkGetDeviceProcAddr( device, "vkAcquireNextImageKHR" ) : vkGetInstanceProcAddr( instance, "vkAcquireNextImageKHR" ) );
      vkAllocateCommandBuffers = PFN_vkAllocateCommandBuffers( device ? vkGetDeviceProcAddr( device, "vkAllocateCommandBuffers" ) : vkGetInstanceProcAddr( instance, "vkAllocateCommandBuffers" ) );
      vkAllocateDescriptorSets = PFN_vkAllocateDescriptorSets( device ? vkGetDeviceProcAddr( device, "vkAllocateDescriptorSets" ) : vkGetInstanceProcAddr( instance, "vkAllocateDescriptorSets" ) );
      vkAllocateMemory = PFN_vkAllocateMemory( device ? vkGetDeviceProcAddr( device, "vkAllocateMemory" ) : vkGetInstanceProcAddr( instance, "vkAllocateMemory" ) );
#if defined(VK_NV_ray_tracing)
      vkBindAccelerationStructureMemoryNV = PFN_vkBindAccelerationStructureMemoryNV( device ? vkGetDeviceProcAddr( device, "vkBindAccelerationStructureMemoryNV" ) : vkGetInstanceProcAddr( instance, "vkBindAccelerationStructureMemoryNV" ) );
#endif
      vkBindBufferMemory = PFN_vkBindBufferMemory( device ? vkGetDeviceProcAddr( device, "vkBindBufferMemory" ) : vkGetInstanceProcAddr( instance, "vkBindBufferMemory" ) );
      vkBindBufferMemory2 = PFN_vkBindBufferMemory2( device ? vkGetDeviceProcAddr( device, "vkBindBufferMemory2" ) : vkGetInstanceProcAddr( instance, "vkBindBufferMemory2" ) );
      vkBindBufferMemory2KHR = PFN_vkBindBufferMemory2KHR( device ? vkGetDeviceProcAddr( device, "vkBindBufferMemory2KHR" ) : vkGetInstanceProcAddr( instance, "vkBindBufferMemory2KHR" ) );
      vkBindImageMemory = PFN_vkBindImageMemory( device ? vkGetDeviceProcAddr( device, "vkBindImageMemory" ) : vkGetInstanceProcAddr( instance, "vkBindImageMemory" ) );
      vkBindImageMemory2 = PFN_vkBindImageMemory2( device ? vkGetDeviceProcAddr( device, "vkBindImageMemory2" ) : vkGetInstanceProcAddr( instance, "vkBindImageMemory2" ) );
      vkBindImageMemory2KHR = PFN_vkBindImageMemory2KHR( device ? vkGetDeviceProcAddr( device, "vkBindImageMemory2KHR" ) : vkGetInstanceProcAddr( instance, "vkBindImageMemory2KHR" ) );
#if defined(VK_NV_ray_tracing)
      vkCompileDeferredNV = PFN_vkCompileDeferredNV( device ? vkGetDeviceProcAddr( device, "vkCompileDeferredNV" ) : vkGetInstanceProcAddr( instance, "vkCompileDeferredNV" ) );
      vkCreateAccelerationStructureNV = PFN_vkCreateAccelerationStructureNV( device ? vkGetDeviceProcAddr( device, "vkCreateAccelerationStructureNV" ) : vkGetInstanceProcAddr( instance, "vkCreateAccelerationStructureNV" ) );
#endif
      vkCreateBuffer = PFN_vkCreateBuffer( device ? vkGetDeviceProcAddr( device, "vkCreateBuffer" ) : vkGetInstanceProcAddr( instance, "vkCreateBuffer" ) );
      vkCreateBufferView = PFN_vkCreateBufferView( device ? vkGetDeviceProcAddr( device, "vkCreateBufferView" ) : vkGetInstanceProcAddr( instance, "vkCreateBufferView" ) );
      vkCreateCommandPool = PFN_vkCreateCommandPool( device ? vkGetDeviceProcAddr( device, "vkCreateCommandPool" ) : vkGetInstanceProcAddr( instance, "vkCreateCommandPool" ) );
      vkCreateComputePipelines = PFN_vkCreateComputePipelines( device ? vkGetDeviceProcAddr( device, "vkCreateComputePipelines" ) : vkGetInstanceProcAddr( instance, "vkCreateComputePipelines" ) );
      vkCreateDescriptorPool = PFN_vkCreateDescriptorPool( device ? vkGetDeviceProcAddr( device, "vkCreateDescriptorPool" ) : vkGetInstanceProcAddr( instance, "vkCreateDescriptorPool" ) );
      vkCreateDescriptorSetLayout = PFN_vkCreateDescriptorSetLayout( device ? vkGetDeviceProcAddr( device, "vkCreateDescriptorSetLayout" ) : vkGetInstanceProcAddr( instance, "vkCreateDescriptorSetLayout" ) );
      vkCreateDescriptorUpdateTemplate = PFN_vkCreateDescriptorUpdateTemplate( device ? vkGetDeviceProcAddr( device, "vkCreateDescriptorUpdateTemplate" ) : vkGetInstanceProcAddr( instance, "vkCreateDescriptorUpdateTemplate" ) );
      vkCreateDescriptorUpdateTemplateKHR = PFN_vkCreateDescriptorUpdateTemplateKHR( device ? vkGetDeviceProcAddr( device, "vkCreateDescriptorUpdateTemplateKHR" ) : vkGetInstanceProcAddr( instance, "vkCreateDescriptorUpdateTemplateKHR" ) );
      vkCreateEvent = PFN_vkCreateEvent( device ? vkGetDeviceProcAddr( device, "vkCreateEvent" ) : vkGetInstanceProcAddr( instance, "vkCreateEvent" ) );
      vkCreateFence = PFN_vkCreateFence( device ? vkGetDeviceProcAddr( device, "vkCreateFence" ) : vkGetInstanceProcAddr( instance, "vkCreateFence" ) );
      vkCreateFramebuffer = PFN_vkCreateFramebuffer( device ? vkGetDeviceProcAddr( device, "vkCreateFramebuffer" ) : vkGetInstanceProcAddr( instance, "vkCreateFramebuffer" ) );
      vkCreateGraphicsPipelines = PFN_vkCreateGraphicsPipelines( device ? vkGetDeviceProcAddr( device, "vkCreateGraphicsPipelines" ) : vkGetInstanceProcAddr( instance, "vkCreateGraphicsPipelines" ) );
      vkCreateImage = PFN_vkCreateImage( device ? vkGetDeviceProcAddr( device, "vkCreateImage" ) : vkGetInstanceProcAddr( instance, "vkCreateImage" ) );
      vkCreateImageView = PFN_vkCreateImageView( device ? vkGetDeviceProcAddr( device, "vkCreateImageView" ) : vkGetInstanceProcAddr( instance, "vkCreateImageView" ) );
      vkCreateIndirectCommandsLayoutNVX = PFN_vkCreateIndirectCommandsLayoutNVX( device ? vkGetDeviceProcAddr( device, "vkCreateIndirectCommandsLayoutNVX" ) : vkGetInstanceProcAddr( instance, "vkCreateIndirectCommandsLayoutNVX" ) );
      vkCreateObjectTableNVX = PFN_vkCreateObjectTableNVX( device ? vkGetDeviceProcAddr( device, "vkCreateObjectTableNVX" ) : vkGetInstanceProcAddr( instance, "vkCreateObjectTableNVX" ) );
      vkCreatePipelineCache = PFN_vkCreatePipelineCache( device ? vkGetDeviceProcAddr( device, "vkCreatePipelineCache" ) : vkGetInstanceProcAddr( instance, "vkCreatePipelineCache" ) );
      vkCreatePipelineLayout = PFN_vkCreatePipelineLayout( device ? vkGetDeviceProcAddr( device, "vkCreatePipelineLayout" ) : vkGetInstanceProcAddr( instance, "vkCreatePipelineLayout" ) );
      vkCreateQueryPool = PFN_vkCreateQueryPool( device ? vkGetDeviceProcAddr( device, "vkCreateQueryPool" ) : vkGetInstanceProcAddr( instance, "vkCreateQueryPool" ) );
#if defined(VK_NV_ray_tracing)
      vkCreateRayTracingPipelinesNV = PFN_vkCreateRayTracingPipelinesNV( device ? vkGetDeviceProcAddr( device, "vkCreateRayTracingPipelinesNV" ) : vkGetInstanceProcAddr( instance, "vkCreateRayTracingPipelinesNV" ) );
#endif
      vkCreateRenderPass = PFN_vkCreateRenderPass( device ? vkGetDeviceProcAddr( device, "vkCreateRenderPass" ) : vkGetInstanceProcAddr( instance, "vkCreateRenderPass" ) );
      vkCreateRenderPass2KHR = PFN_vkCreateRenderPass2KHR( device ? vkGetDeviceProcAddr( device, "vkCreateRenderPass2KHR" ) : vkGetInstanceProcAddr( instance, "vkCreateRenderPass2KHR" ) );
      vkCreateSampler = PFN_vkCreateSampler( device ? vkGetDeviceProcAddr( device, "vkCreateSampler" ) : vkGetInstanceProcAddr( instance, "vkCreateSampler" ) );
      vkCreateSamplerYcbcrConversion = PFN_vkCreateSamplerYcbcrConversion( device ? vkGetDeviceProcAddr( device, "vkCreateSamplerYcbcrConversion" ) : vkGetInstanceProcAddr( instance, "vkCreateSamplerYcbcrConversion" ) );
      vkCreateSamplerYcbcrConversionKHR = PFN_vkCreateSamplerYcbcrConversionKHR( device ? vkGetDeviceProcAddr( device, "vkCreateSamplerYcbcrConversionKHR" ) : vkGetInstanceProcAddr( instance, "vkCreateSamplerYcbcrConversionKHR" ) );
      vkCreateSemaphore = PFN_vkCreateSemaphore( device ? vkGetDeviceProcAddr( device, "vkCreateSemaphore" ) : vkGetInstanceProcAddr( instance, "vkCreateSemaphore" ) );
      vkCreateShaderModule = PFN_vkCreateShaderModule( device ? vkGetDeviceProcAddr( device, "vkCreateShaderModule" ) : vkGetInstanceProcAddr( instance, "vkCreateShaderModule" ) );
      vkCreateSharedSwapchainsKHR = PFN_vkCreateSharedSwapchainsKHR( device ? vkGetDeviceProcAddr( device, "vkCreateSharedSwapchainsKHR" ) : vkGetInstanceProcAddr( instance, "vkCreateSharedSwapchainsKHR" ) );
      vkCreateSwapchainKHR = PFN_vkCreateSwapchainKHR( device ? vkGetDeviceProcAddr( device, "vkCreateSwapchainKHR" ) : vkGetInstanceProcAddr( instance, "vkCreateSwapchainKHR" ) );
      vkCreateValidationCacheEXT = PFN_vkCreateValidationCacheEXT( device ? vkGetDeviceProcAddr( device, "vkCreateValidationCacheEXT" ) : vkGetInstanceProcAddr( instance, "vkCreateValidationCacheEXT" ) );
      vkDebugMarkerSetObjectNameEXT = PFN_vkDebugMarkerSetObjectNameEXT( device ? vkGetDeviceProcAddr( device, "vkDebugMarkerSetObjectNameEXT" ) : vkGetInstanceProcAddr( instance, "vkDebugMarkerSetObjectNameEXT" ) );
      vkDebugMarkerSetObjectTagEXT = PFN_vkDebugMarkerSetObjectTagEXT( device ? vkGetDeviceProcAddr( device, "vkDebugMarkerSetObjectTagEXT" ) : vkGetInstanceProcAddr( instance, "vkDebugMarkerSetObjectTagEXT" ) );
#if defined(VK_NV_ray_tracing)
      vkDestroyAccelerationStructureNV = PFN_vkDestroyAccelerationStructureNV( device ? vkGetDeviceProcAddr( device, "vkDestroyAccelerationStructureNV" ) : vkGetInstanceProcAddr( instance, "vkDestroyAccelerationStructureNV" ) );
#endif
      vkDestroyBuffer = PFN_vkDestroyBuffer( device ? vkGetDeviceProcAddr( device, "vkDestroyBuffer" ) : vkGetInstanceProcAddr( instance, "vkDestroyBuffer" ) );
      vkDestroyBufferView = PFN_vkDestroyBufferView( device ? vkGetDeviceProcAddr( device, "vkDestroyBufferView" ) : vkGetInstanceProcAddr( instance, "vkDestroyBufferView" ) );
      vkDestroyCommandPool = PFN_vkDestroyCommandPool( device ? vkGetDeviceProcAddr( device, "vkDestroyCommandPool" ) : vkGetInstanceProcAddr( instance, "vkDestroyCommandPool" ) );
      vkDestroyDescriptorPool = PFN_vkDestroyDescriptorPool( device ? vkGetDeviceProcAddr( device, "vkDestroyDescriptorPool" ) : vkGetInstanceProcAddr( instance, "vkDestroyDescriptorPool" ) );
      vkDestroyDescriptorSetLayout = PFN_vkDestroyDescriptorSetLayout( device ? vkGetDeviceProcAddr( device, "vkDestroyDescriptorSetLayout" ) : vkGetInstanceProcAddr( instance, "vkDestroyDescriptorSetLayout" ) );
      vkDestroyDescriptorUpdateTemplate = PFN_vkDestroyDescriptorUpdateTemplate( device ? vkGetDeviceProcAddr( device, "vkDestroyDescriptorUpdateTemplate" ) : vkGetInstanceProcAddr( instance, "vkDestroyDescriptorUpdateTemplate" ) );
      vkDestroyDescriptorUpdateTemplateKHR = PFN_vkDestroyDescriptorUpdateTemplateKHR( device ? vkGetDeviceProcAddr( device, "vkDestroyDescriptorUpdateTemplateKHR" ) : vkGetInstanceProcAddr( instance, "vkDestroyDescriptorUpdateTemplateKHR" ) );
      vkDestroyDevice = PFN_vkDestroyDevice( device ? vkGetDeviceProcAddr( device, "vkDestroyDevice" ) : vkGetInstanceProcAddr( instance, "vkDestroyDevice" ) );
      vkDestroyEvent = PFN_vkDestroyEvent( device ? vkGetDeviceProcAddr( device, "vkDestroyEvent" ) : vkGetInstanceProcAddr( instance, "vkDestroyEvent" ) );
      vkDestroyFence = PFN_vkDestroyFence( device ? vkGetDeviceProcAddr( device, "vkDestroyFence" ) : vkGetInstanceProcAddr( instance, "vkDestroyFence" ) );
      vkDestroyFramebuffer = PFN_vkDestroyFramebuffer( device ? vkGetDeviceProcAddr( device, "vkDestroyFramebuffer" ) : vkGetInstanceProcAddr( instance, "vkDestroyFramebuffer" ) );
      vkDestroyImage = PFN_vkDestroyImage( device ? vkGetDeviceProcAddr( device, "vkDestroyImage" ) : vkGetInstanceProcAddr( instance, "vkDestroyImage" ) );
      vkDestroyImageView = PFN_vkDestroyImageView( device ? vkGetDeviceProcAddr( device, "vkDestroyImageView" ) : vkGetInstanceProcAddr( instance, "vkDestroyImageView" ) );
      vkDestroyIndirectCommandsLayoutNVX = PFN_vkDestroyIndirectCommandsLayoutNVX( device ? vkGetDeviceProcAddr( device, "vkDestroyIndirectCommandsLayoutNVX" ) : vkGetInstanceProcAddr( instance, "vkDestroyIndirectCommandsLayoutNVX" ) );
      vkDestroyObjectTableNVX = PFN_vkDestroyObjectTableNVX( device ? vkGetDeviceProcAddr( device, "vkDestroyObjectTableNVX" ) : vkGetInstanceProcAddr( instance, "vkDestroyObjectTableNVX" ) );
      vkDestroyPipeline = PFN_vkDestroyPipeline( device ? vkGetDeviceProcAddr( device, "vkDestroyPipeline" ) : vkGetInstanceProcAddr( instance, "vkDestroyPipeline" ) );
      vkDestroyPipelineCache = PFN_vkDestroyPipelineCache( device ? vkGetDeviceProcAddr( device, "vkDestroyPipelineCache" ) : vkGetInstanceProcAddr( instance, "vkDestroyPipelineCache" ) );
      vkDestroyPipelineLayout = PFN_vkDestroyPipelineLayout( device ? vkGetDeviceProcAddr( device, "vkDestroyPipelineLayout" ) : vkGetInstanceProcAddr( instance, "vkDestroyPipelineLayout" ) );
      vkDestroyQueryPool = PFN_vkDestroyQueryPool( device ? vkGetDeviceProcAddr( device, "vkDestroyQueryPool" ) : vkGetInstanceProcAddr( instance, "vkDestroyQueryPool" ) );
      vkDestroyRenderPass = PFN_vkDestroyRenderPass( device ? vkGetDeviceProcAddr( device, "vkDestroyRenderPass" ) : vkGetInstanceProcAddr( instance, "vkDestroyRenderPass" ) );
      vkDestroySampler = PFN_vkDestroySampler( device ? vkGetDeviceProcAddr( device, "vkDestroySampler" ) : vkGetInstanceProcAddr( instance, "vkDestroySampler" ) );
      vkDestroySamplerYcbcrConversion = PFN_vkDestroySamplerYcbcrConversion( device ? vkGetDeviceProcAddr( device, "vkDestroySamplerYcbcrConversion" ) : vkGetInstanceProcAddr( instance, "vkDestroySamplerYcbcrConversion" ) );
      vkDestroySamplerYcbcrConversionKHR = PFN_vkDestroySamplerYcbcrConversionKHR( device ? vkGetDeviceProcAddr( device, "vkDestroySamplerYcbcrConversionKHR" ) : vkGetInstanceProcAddr( instance, "vkDestroySamplerYcbcrConversionKHR" ) );
      vkDestroySemaphore = PFN_vkDestroySemaphore( device ? vkGetDeviceProcAddr( device, "vkDestroySemaphore" ) : vkGetInstanceProcAddr( instance, "vkDestroySemaphore" ) );
      vkDestroyShaderModule = PFN_vkDestroyShaderModule( device ? vkGetDeviceProcAddr( device, "vkDestroyShaderModule" ) : vkGetInstanceProcAddr( instance, "vkDestroyShaderModule" ) );
      vkDestroySwapchainKHR = PFN_vkDestroySwapchainKHR( device ? vkGetDeviceProcAddr( device, "vkDestroySwapchainKHR" ) : vkGetInstanceProcAddr( instance, "vkDestroySwapchainKHR" ) );
      vkDestroyValidationCacheEXT = PFN_vkDestroyValidationCacheEXT( device ? vkGetDeviceProcAddr( device, "vkDestroyValidationCacheEXT" ) : vkGetInstanceProcAddr( instance, "vkDestroyValidationCacheEXT" ) );
      vkDeviceWaitIdle = PFN_vkDeviceWaitIdle( device ? vkGetDeviceProcAddr( device, "vkDeviceWaitIdle" ) : vkGetInstanceProcAddr( instance, "vkDeviceWaitIdle" ) );
      vkDisplayPowerControlEXT = PFN_vkDisplayPowerControlEXT( device ? vkGetDeviceProcAddr( device, "vkDisplayPowerControlEXT" ) : vkGetInstanceProcAddr( instance, "vkDisplayPowerControlEXT" ) );
      vkFlushMappedMemoryRanges = PFN_vkFlushMappedMemoryRanges( device ? vkGetDeviceProcAddr( device, "vkFlushMappedMemoryRanges" ) : vkGetInstanceProcAddr( instance, "vkFlushMappedMemoryRanges" ) );
      vkFreeCommandBuffers = PFN_vkFreeCommandBuffers( device ? vkGetDeviceProcAddr( device, "vkFreeCommandBuffers" ) : vkGetInstanceProcAddr( instance, "vkFreeCommandBuffers" ) );
      vkFreeDescriptorSets = PFN_vkFreeDescriptorSets( device ? vkGetDeviceProcAddr( device, "vkFreeDescriptorSets" ) : vkGetInstanceProcAddr( instance, "vkFreeDescriptorSets" ) );
      vkFreeMemory = PFN_vkFreeMemory( device ? vkGetDeviceProcAddr( device, "vkFreeMemory" ) : vkGetInstanceProcAddr( instance, "vkFreeMemory" ) );
#if defined(VK_NV_ray_tracing)
      vkGetAccelerationStructureHandleNV = PFN_vkGetAccelerationStructureHandleNV( device ? vkGetDeviceProcAddr( device, "vkGetAccelerationStructureHandleNV" ) : vkGetInstanceProcAddr( instance, "vkGetAccelerationStructureHandleNV" ) );
      vkGetAccelerationStructureMemoryRequirementsNV = PFN_vkGetAccelerationStructureMemoryRequirementsNV( device ? vkGetDeviceProcAddr( device, "vkGetAccelerationStructureMemoryRequirementsNV" ) : vkGetInstanceProcAddr( instance, "vkGetAccelerationStructureMemoryRequirementsNV" ) );
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
      vkGetAndroidHardwareBufferPropertiesANDROID = PFN_vkGetAndroidHardwareBufferPropertiesANDROID( device ? vkGetDeviceProcAddr( device, "vkGetAndroidHardwareBufferPropertiesANDROID" ) : vkGetInstanceProcAddr( instance, "vkGetAndroidHardwareBufferPropertiesANDROID" ) );
#endif
#endif /*VK_USE_PLATFORM_ANDROID_KHR*/
#if defined(VK_EXT_buffer_device_address)
      vkGetBufferDeviceAddressEXT = PFN_vkGetBufferDeviceAddressEXT( device ? vkGetDeviceProcAddr( device, "vkGetBufferDeviceAddressEXT" ) : vkGetInstanceProcAddr( instance, "vkGetBufferDeviceAddressEXT" ) );
#endif
      vkGetBufferMemoryRequirements = PFN_vkGetBufferMemoryRequirements( device ? vkGetDeviceProcAddr( device, "vkGetBufferMemoryRequirements" ) : vkGetInstanceProcAddr( instance, "vkGetBufferMemoryRequirements" ) );
      vkGetBufferMemoryRequirements2 = PFN_vkGetBufferMemoryRequirements2( device ? vkGetDeviceProcAddr( device, "vkGetBufferMemoryRequirements2" ) : vkGetInstanceProcAddr( instance, "vkGetBufferMemoryRequirements2" ) );
      vkGetBufferMemoryRequirements2KHR = PFN_vkGetBufferMemoryRequirements2KHR( device ? vkGetDeviceProcAddr( device, "vkGetBufferMemoryRequirements2KHR" ) : vkGetInstanceProcAddr( instance, "vkGetBufferMemoryRequirements2KHR" ) );
#if defined(VK_EXT_calibrated_timestamps)
      vkGetCalibratedTimestampsEXT = PFN_vkGetCalibratedTimestampsEXT( device ? vkGetDeviceProcAddr( device, "vkGetCalibratedTimestampsEXT" ) : vkGetInstanceProcAddr( instance, "vkGetCalibratedTimestampsEXT" ) );
#endif
      vkGetDescriptorSetLayoutSupport = PFN_vkGetDescriptorSetLayoutSupport( device ? vkGetDeviceProcAddr( device, "vkGetDescriptorSetLayoutSupport" ) : vkGetInstanceProcAddr( instance, "vkGetDescriptorSetLayoutSupport" ) );
#if defined(VK_KHR_maintenance3)
      vkGetDescriptorSetLayoutSupportKHR = PFN_vkGetDescriptorSetLayoutSupportKHR( device ? vkGetDeviceProcAddr( device, "vkGetDescriptorSetLayoutSupportKHR" ) : vkGetInstanceProcAddr( instance, "vkGetDescriptorSetLayoutSupportKHR" ) );
#endif
      vkGetDeviceGroupPeerMemoryFeatures = PFN_vkGetDeviceGroupPeerMemoryFeatures( device ? vkGetDeviceProcAddr( device, "vkGetDeviceGroupPeerMemoryFeatures" ) : vkGetInstanceProcAddr( instance, "vkGetDeviceGroupPeerMemoryFeatures" ) );
      vkGetDeviceGroupPeerMemoryFeaturesKHR = PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR( device ? vkGetDeviceProcAddr( device, "vkGetDeviceGroupPeerMemoryFeaturesKHR" ) : vkGetInstanceProcAddr( instance, "vkGetDeviceGroupPeerMemoryFeaturesKHR" ) );
      vkGetDeviceGroupPresentCapabilitiesKHR = PFN_vkGetDeviceGroupPresentCapabilitiesKHR( device ? vkGetDeviceProcAddr( device, "vkGetDeviceGroupPresentCapabilitiesKHR" ) : vkGetInstanceProcAddr( instance, "vkGetDeviceGroupPresentCapabilitiesKHR" ) );
#ifdef VK_USE_PLATFORM_WIN32_KHR
      vkGetDeviceGroupSurfacePresentModes2EXT = PFN_vkGetDeviceGroupSurfacePresentModes2EXT( device ? vkGetDeviceProcAddr( device, "vkGetDeviceGroupSurfacePresentModes2EXT" ) : vkGetInstanceProcAddr( instance, "vkGetDeviceGroupSurfacePresentModes2EXT" ) );
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
      vkGetDeviceGroupSurfacePresentModesKHR = PFN_vkGetDeviceGroupSurfacePresentModesKHR( device ? vkGetDeviceProcAddr( device, "vkGetDeviceGroupSurfacePresentModesKHR" ) : vkGetInstanceProcAddr( instance, "vkGetDeviceGroupSurfacePresentModesKHR" ) );
      vkGetDeviceMemoryCommitment = PFN_vkGetDeviceMemoryCommitment( device ? vkGetDeviceProcAddr( device, "vkGetDeviceMemoryCommitment" ) : vkGetInstanceProcAddr( instance, "vkGetDeviceMemoryCommitment" ) );
      vkGetDeviceQueue = PFN_vkGetDeviceQueue( device ? vkGetDeviceProcAddr( device, "vkGetDeviceQueue" ) : vkGetInstanceProcAddr( instance, "vkGetDeviceQueue" ) );
      vkGetDeviceQueue2 = PFN_vkGetDeviceQueue2( device ? vkGetDeviceProcAddr( device, "vkGetDeviceQueue2" ) : vkGetInstanceProcAddr( instance, "vkGetDeviceQueue2" ) );
      vkGetEventStatus = PFN_vkGetEventStatus( device ? vkGetDeviceProcAddr( device, "vkGetEventStatus" ) : vkGetInstanceProcAddr( instance, "vkGetEventStatus" ) );
      vkGetFenceFdKHR = PFN_vkGetFenceFdKHR( device ? vkGetDeviceProcAddr( device, "vkGetFenceFdKHR" ) : vkGetInstanceProcAddr( instance, "vkGetFenceFdKHR" ) );
      vkGetFenceStatus = PFN_vkGetFenceStatus( device ? vkGetDeviceProcAddr( device, "vkGetFenceStatus" ) : vkGetInstanceProcAddr( instance, "vkGetFenceStatus" ) );
#ifdef VK_USE_PLATFORM_WIN32_KHR
      vkGetFenceWin32HandleKHR = PFN_vkGetFenceWin32HandleKHR( device ? vkGetDeviceProcAddr( device, "vkGetFenceWin32HandleKHR" ) : vkGetInstanceProcAddr( instance, "vkGetFenceWin32HandleKHR" ) );
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
#if defined(VK_EXT_image_drm_format_modifier)
      vkGetImageDrmFormatModifierPropertiesEXT = PFN_vkGetImageDrmFormatModifierPropertiesEXT( device ? vkGetDeviceProcAddr( device, "vkGetImageDrmFormatModifierPropertiesEXT" ) : vkGetInstanceProcAddr( instance, "vkGetImageDrmFormatModifierPropertiesEXT" ) );
#endif
      vkGetImageMemoryRequirements = PFN_vkGetImageMemoryRequirements( device ? vkGetDeviceProcAddr( device, "vkGetImageMemoryRequirements" ) : vkGetInstanceProcAddr( instance, "vkGetImageMemoryRequirements" ) );
      vkGetImageMemoryRequirements2 = PFN_vkGetImageMemoryRequirements2( device ? vkGetDeviceProcAddr( device, "vkGetImageMemoryRequirements2" ) : vkGetInstanceProcAddr( instance, "vkGetImageMemoryRequirements2" ) );
      vkGetImageMemoryRequirements2KHR = PFN_vkGetImageMemoryRequirements2KHR( device ? vkGetDeviceProcAddr( device, "vkGetImageMemoryRequirements2KHR" ) : vkGetInstanceProcAddr( instance, "vkGetImageMemoryRequirements2KHR" ) );
      vkGetImageSparseMemoryRequirements = PFN_vkGetImageSparseMemoryRequirements( device ? vkGetDeviceProcAddr( device, "vkGetImageSparseMemoryRequirements" ) : vkGetInstanceProcAddr( instance, "vkGetImageSparseMemoryRequirements" ) );
      vkGetImageSparseMemoryRequirements2 = PFN_vkGetImageSparseMemoryRequirements2( device ? vkGetDeviceProcAddr( device, "vkGetImageSparseMemoryRequirements2" ) : vkGetInstanceProcAddr( instance, "vkGetImageSparseMemoryRequirements2" ) );
      vkGetImageSparseMemoryRequirements2KHR = PFN_vkGetImageSparseMemoryRequirements2KHR( device ? vkGetDeviceProcAddr( device, "vkGetImageSparseMemoryRequirements2KHR" ) : vkGetInstanceProcAddr( instance, "vkGetImageSparseMemoryRequirements2KHR" ) );
      vkGetImageSubresourceLayout = PFN_vkGetImageSubresourceLayout( device ? vkGetDeviceProcAddr( device, "vkGetImageSubresourceLayout" ) : vkGetInstanceProcAddr( instance, "vkGetImageSubresourceLayout" ) );
#if defined(VK_NVX_image_view_handle)
      vkGetImageViewHandleNVX = PFN_vkGetImageViewHandleNVX( device ? vkGetDeviceProcAddr( device, "vkGetImageViewHandleNVX" ) : vkGetInstanceProcAddr( instance, "vkGetImageViewHandleNVX" ) );
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
      vkGetMemoryAndroidHardwareBufferANDROID = PFN_vkGetMemoryAndroidHardwareBufferANDROID( device ? vkGetDeviceProcAddr( device, "vkGetMemoryAndroidHardwareBufferANDROID" ) : vkGetInstanceProcAddr( instance, "vkGetMemoryAndroidHardwareBufferANDROID" ) );
#endif
#endif /*VK_USE_PLATFORM_ANDROID_KHR*/
      vkGetMemoryFdKHR = PFN_vkGetMemoryFdKHR( device ? vkGetDeviceProcAddr( device, "vkGetMemoryFdKHR" ) : vkGetInstanceProcAddr( instance, "vkGetMemoryFdKHR" ) );
      vkGetMemoryFdPropertiesKHR = PFN_vkGetMemoryFdPropertiesKHR( device ? vkGetDeviceProcAddr( device, "vkGetMemoryFdPropertiesKHR" ) : vkGetInstanceProcAddr( instance, "vkGetMemoryFdPropertiesKHR" ) );
      vkGetMemoryHostPointerPropertiesEXT = PFN_vkGetMemoryHostPointerPropertiesEXT( device ? vkGetDeviceProcAddr( device, "vkGetMemoryHostPointerPropertiesEXT" ) : vkGetInstanceProcAddr( instance, "vkGetMemoryHostPointerPropertiesEXT" ) );
#ifdef VK_USE_PLATFORM_WIN32_KHR
      vkGetMemoryWin32HandleKHR = PFN_vkGetMemoryWin32HandleKHR( device ? vkGetDeviceProcAddr( device, "vkGetMemoryWin32HandleKHR" ) : vkGetInstanceProcAddr( instance, "vkGetMemoryWin32HandleKHR" ) );
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
#ifdef VK_USE_PLATFORM_WIN32_KHR
      vkGetMemoryWin32HandleNV = PFN_vkGetMemoryWin32HandleNV( device ? vkGetDeviceProcAddr( device, "vkGetMemoryWin32HandleNV" ) : vkGetInstanceProcAddr( instance, "vkGetMemoryWin32HandleNV" ) );
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
#ifdef VK_USE_PLATFORM_WIN32_KHR
      vkGetMemoryWin32HandlePropertiesKHR = PFN_vkGetMemoryWin32HandlePropertiesKHR( device ? vkGetDeviceProcAddr( device, "vkGetMemoryWin32HandlePropertiesKHR" ) : vkGetInstanceProcAddr( instance, "vkGetMemoryWin32HandlePropertiesKHR" ) );
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
      vkGetPastPresentationTimingGOOGLE = PFN_vkGetPastPresentationTimingGOOGLE( device ? vkGetDeviceProcAddr( device, "vkGetPastPresentationTimingGOOGLE" ) : vkGetInstanceProcAddr( instance, "vkGetPastPresentationTimingGOOGLE" ) );
      vkGetPipelineCacheData = PFN_vkGetPipelineCacheData( device ? vkGetDeviceProcAddr( device, "vkGetPipelineCacheData" ) : vkGetInstanceProcAddr( instance, "vkGetPipelineCacheData" ) );
      vkGetQueryPoolResults = PFN_vkGetQueryPoolResults( device ? vkGetDeviceProcAddr( device, "vkGetQueryPoolResults" ) : vkGetInstanceProcAddr( instance, "vkGetQueryPoolResults" ) );
#if defined(VK_NV_ray_tracing)
      vkGetRayTracingShaderGroupHandlesNV = PFN_vkGetRayTracingShaderGroupHandlesNV( device ? vkGetDeviceProcAddr( device, "vkGetRayTracingShaderGroupHandlesNV" ) : vkGetInstanceProcAddr( instance, "vkGetRayTracingShaderGroupHandlesNV" ) );
#endif
      vkGetRefreshCycleDurationGOOGLE = PFN_vkGetRefreshCycleDurationGOOGLE( device ? vkGetDeviceProcAddr( device, "vkGetRefreshCycleDurationGOOGLE" ) : vkGetInstanceProcAddr( instance, "vkGetRefreshCycleDurationGOOGLE" ) );
      vkGetRenderAreaGranularity = PFN_vkGetRenderAreaGranularity( device ? vkGetDeviceProcAddr( device, "vkGetRenderAreaGranularity" ) : vkGetInstanceProcAddr( instance, "vkGetRenderAreaGranularity" ) );
      vkGetSemaphoreFdKHR = PFN_vkGetSemaphoreFdKHR( device ? vkGetDeviceProcAddr( device, "vkGetSemaphoreFdKHR" ) : vkGetInstanceProcAddr( instance, "vkGetSemaphoreFdKHR" ) );
#ifdef VK_USE_PLATFORM_WIN32_KHR
      vkGetSemaphoreWin32HandleKHR = PFN_vkGetSemaphoreWin32HandleKHR( device ? vkGetDeviceProcAddr( device, "vkGetSemaphoreWin32HandleKHR" ) : vkGetInstanceProcAddr( instance, "vkGetSemaphoreWin32HandleKHR" ) );
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
      vkGetShaderInfoAMD = PFN_vkGetShaderInfoAMD( device ? vkGetDeviceProcAddr( device, "vkGetShaderInfoAMD" ) : vkGetInstanceProcAddr( instance, "vkGetShaderInfoAMD" ) );
      vkGetSwapchainCounterEXT = PFN_vkGetSwapchainCounterEXT( device ? vkGetDeviceProcAddr( device, "vkGetSwapchainCounterEXT" ) : vkGetInstanceProcAddr( instance, "vkGetSwapchainCounterEXT" ) );
      vkGetSwapchainImagesKHR = PFN_vkGetSwapchainImagesKHR( device ? vkGetDeviceProcAddr( device, "vkGetSwapchainImagesKHR" ) : vkGetInstanceProcAddr( instance, "vkGetSwapchainImagesKHR" ) );
      vkGetSwapchainStatusKHR = PFN_vkGetSwapchainStatusKHR( device ? vkGetDeviceProcAddr( device, "vkGetSwapchainStatusKHR" ) : vkGetInstanceProcAddr( instance, "vkGetSwapchainStatusKHR" ) );
      vkGetValidationCacheDataEXT = PFN_vkGetValidationCacheDataEXT( device ? vkGetDeviceProcAddr( device, "vkGetValidationCacheDataEXT" ) : vkGetInstanceProcAddr( instance, "vkGetValidationCacheDataEXT" ) );
      vkImportFenceFdKHR = PFN_vkImportFenceFdKHR( device ? vkGetDeviceProcAddr( device, "vkImportFenceFdKHR" ) : vkGetInstanceProcAddr( instance, "vkImportFenceFdKHR" ) );
#ifdef VK_USE_PLATFORM_WIN32_KHR
      vkImportFenceWin32HandleKHR = PFN_vkImportFenceWin32HandleKHR( device ? vkGetDeviceProcAddr( device, "vkImportFenceWin32HandleKHR" ) : vkGetInstanceProcAddr( instance, "vkImportFenceWin32HandleKHR" ) );
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
      vkImportSemaphoreFdKHR = PFN_vkImportSemaphoreFdKHR( device ? vkGetDeviceProcAddr( device, "vkImportSemaphoreFdKHR" ) : vkGetInstanceProcAddr( instance, "vkImportSemaphoreFdKHR" ) );
#ifdef VK_USE_PLATFORM_WIN32_KHR
      vkImportSemaphoreWin32HandleKHR = PFN_vkImportSemaphoreWin32HandleKHR( device ? vkGetDeviceProcAddr( device, "vkImportSemaphoreWin32HandleKHR" ) : vkGetInstanceProcAddr( instance, "vkImportSemaphoreWin32HandleKHR" ) );
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
      vkInvalidateMappedMemoryRanges = PFN_vkInvalidateMappedMemoryRanges( device ? vkGetDeviceProcAddr( device, "vkInvalidateMappedMemoryRanges" ) : vkGetInstanceProcAddr( instance, "vkInvalidateMappedMemoryRanges" ) );
      vkMapMemory = PFN_vkMapMemory( device ? vkGetDeviceProcAddr( device, "vkMapMemory" ) : vkGetInstanceProcAddr( instance, "vkMapMemory" ) );
      vkMergePipelineCaches = PFN_vkMergePipelineCaches( device ? vkGetDeviceProcAddr( device, "vkMergePipelineCaches" ) : vkGetInstanceProcAddr( instance, "vkMergePipelineCaches" ) );
      vkMergeValidationCachesEXT = PFN_vkMergeValidationCachesEXT( device ? vkGetDeviceProcAddr( device, "vkMergeValidationCachesEXT" ) : vkGetInstanceProcAddr( instance, "vkMergeValidationCachesEXT" ) );
      vkRegisterDeviceEventEXT = PFN_vkRegisterDeviceEventEXT( device ? vkGetDeviceProcAddr( device, "vkRegisterDeviceEventEXT" ) : vkGetInstanceProcAddr( instance, "vkRegisterDeviceEventEXT" ) );
      vkRegisterDisplayEventEXT = PFN_vkRegisterDisplayEventEXT( device ? vkGetDeviceProcAddr( device, "vkRegisterDisplayEventEXT" ) : vkGetInstanceProcAddr( instance, "vkRegisterDisplayEventEXT" ) );
      vkRegisterObjectsNVX = PFN_vkRegisterObjectsNVX( device ? vkGetDeviceProcAddr( device, "vkRegisterObjectsNVX" ) : vkGetInstanceProcAddr( instance, "vkRegisterObjectsNVX" ) );
#ifdef VK_USE_PLATFORM_WIN32_KHR
      vkReleaseFullScreenExclusiveModeEXT = PFN_vkReleaseFullScreenExclusiveModeEXT( device ? vkGetDeviceProcAddr( device, "vkReleaseFullScreenExclusiveModeEXT" ) : vkGetInstanceProcAddr( instance, "vkReleaseFullScreenExclusiveModeEXT" ) );
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
      vkResetCommandPool = PFN_vkResetCommandPool( device ? vkGetDeviceProcAddr( device, "vkResetCommandPool" ) : vkGetInstanceProcAddr( instance, "vkResetCommandPool" ) );
      vkResetDescriptorPool = PFN_vkResetDescriptorPool( device ? vkGetDeviceProcAddr( device, "vkResetDescriptorPool" ) : vkGetInstanceProcAddr( instance, "vkResetDescriptorPool" ) );
      vkResetEvent = PFN_vkResetEvent( device ? vkGetDeviceProcAddr( device, "vkResetEvent" ) : vkGetInstanceProcAddr( instance, "vkResetEvent" ) );
      vkResetFences = PFN_vkResetFences( device ? vkGetDeviceProcAddr( device, "vkResetFences" ) : vkGetInstanceProcAddr( instance, "vkResetFences" ) );
#if defined(VK_EXT_host_query_reset)
      vkResetQueryPoolEXT = PFN_vkResetQueryPoolEXT( device ? vkGetDeviceProcAddr( device, "vkResetQueryPoolEXT" ) : vkGetInstanceProcAddr( instance, "vkResetQueryPoolEXT" ) );
#endif
      vkSetDebugUtilsObjectNameEXT = PFN_vkSetDebugUtilsObjectNameEXT( device ? vkGetDeviceProcAddr( device, "vkSetDebugUtilsObjectNameEXT" ) : vkGetInstanceProcAddr( instance, "vkSetDebugUtilsObjectNameEXT" ) );
      vkSetDebugUtilsObjectTagEXT = PFN_vkSetDebugUtilsObjectTagEXT( device ? vkGetDeviceProcAddr( device, "vkSetDebugUtilsObjectTagEXT" ) : vkGetInstanceProcAddr( instance, "vkSetDebugUtilsObjectTagEXT" ) );
      vkSetEvent = PFN_vkSetEvent( device ? vkGetDeviceProcAddr( device, "vkSetEvent" ) : vkGetInstanceProcAddr( instance, "vkSetEvent" ) );
      vkSetHdrMetadataEXT = PFN_vkSetHdrMetadataEXT( device ? vkGetDeviceProcAddr( device, "vkSetHdrMetadataEXT" ) : vkGetInstanceProcAddr( instance, "vkSetHdrMetadataEXT" ) );
#if defined(VK_AMD_display_native_hdr)
      vkSetLocalDimmingAMD = PFN_vkSetLocalDimmingAMD( device ? vkGetDeviceProcAddr( device, "vkSetLocalDimmingAMD" ) : vkGetInstanceProcAddr( instance, "vkSetLocalDimmingAMD" ) );
#endif
      vkTrimCommandPool = PFN_vkTrimCommandPool( device ? vkGetDeviceProcAddr( device, "vkTrimCommandPool" ) : vkGetInstanceProcAddr( instance, "vkTrimCommandPool" ) );
      vkTrimCommandPoolKHR = PFN_vkTrimCommandPoolKHR( device ? vkGetDeviceProcAddr( device, "vkTrimCommandPoolKHR" ) : vkGetInstanceProcAddr( instance, "vkTrimCommandPoolKHR" ) );
      vkUnmapMemory = PFN_vkUnmapMemory( device ? vkGetDeviceProcAddr( device, "vkUnmapMemory" ) : vkGetInstanceProcAddr( instance, "vkUnmapMemory" ) );
      vkUnregisterObjectsNVX = PFN_vkUnregisterObjectsNVX( device ? vkGetDeviceProcAddr( device, "vkUnregisterObjectsNVX" ) : vkGetInstanceProcAddr( instance, "vkUnregisterObjectsNVX" ) );
      vkUpdateDescriptorSetWithTemplate = PFN_vkUpdateDescriptorSetWithTemplate( device ? vkGetDeviceProcAddr( device, "vkUpdateDescriptorSetWithTemplate" ) : vkGetInstanceProcAddr( instance, "vkUpdateDescriptorSetWithTemplate" ) );
      vkUpdateDescriptorSetWithTemplateKHR = PFN_vkUpdateDescriptorSetWithTemplateKHR( device ? vkGetDeviceProcAddr( device, "vkUpdateDescriptorSetWithTemplateKHR" ) : vkGetInstanceProcAddr( instance, "vkUpdateDescriptorSetWithTemplateKHR" ) );
      vkUpdateDescriptorSets = PFN_vkUpdateDescriptorSets( device ? vkGetDeviceProcAddr( device, "vkUpdateDescriptorSets" ) : vkGetInstanceProcAddr( instance, "vkUpdateDescriptorSets" ) );
      vkWaitForFences = PFN_vkWaitForFences( device ? vkGetDeviceProcAddr( device, "vkWaitForFences" ) : vkGetInstanceProcAddr( instance, "vkWaitForFences" ) );
#ifdef VK_USE_PLATFORM_ANDROID_KHR
      vkCreateAndroidSurfaceKHR = PFN_vkCreateAndroidSurfaceKHR( vkGetInstanceProcAddr( instance, "vkCreateAndroidSurfaceKHR" ) );
#endif /*VK_USE_PLATFORM_ANDROID_KHR*/
      vkCreateDebugReportCallbackEXT = PFN_vkCreateDebugReportCallbackEXT( vkGetInstanceProcAddr( instance, "vkCreateDebugReportCallbackEXT" ) );
      vkCreateDebugUtilsMessengerEXT = PFN_vkCreateDebugUtilsMessengerEXT( vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" ) );
      vkCreateDisplayPlaneSurfaceKHR = PFN_vkCreateDisplayPlaneSurfaceKHR( vkGetInstanceProcAddr( instance, "vkCreateDisplayPlaneSurfaceKHR" ) );
#if defined(VK_EXT_headless_surface)
      vkCreateHeadlessSurfaceEXT = PFN_vkCreateHeadlessSurfaceEXT( vkGetInstanceProcAddr( instance, "vkCreateHeadlessSurfaceEXT" ) );
#endif
#ifdef VK_USE_PLATFORM_IOS_MVK
      vkCreateIOSSurfaceMVK = PFN_vkCreateIOSSurfaceMVK( vkGetInstanceProcAddr( instance, "vkCreateIOSSurfaceMVK" ) );
#endif /*VK_USE_PLATFORM_IOS_MVK*/
#ifdef VK_USE_PLATFORM_FUCHSIA
      vkCreateImagePipeSurfaceFUCHSIA = PFN_vkCreateImagePipeSurfaceFUCHSIA( vkGetInstanceProcAddr( instance, "vkCreateImagePipeSurfaceFUCHSIA" ) );
#endif /*VK_USE_PLATFORM_FUCHSIA*/
#ifdef VK_USE_PLATFORM_MACOS_MVK
      vkCreateMacOSSurfaceMVK = PFN_vkCreateMacOSSurfaceMVK( vkGetInstanceProcAddr( instance, "vkCreateMacOSSurfaceMVK" ) );
#endif /*VK_USE_PLATFORM_MACOS_MVK*/
#ifdef VK_USE_PLATFORM_METAL_EXT
      vkCreateMetalSurfaceEXT = PFN_vkCreateMetalSurfaceEXT( vkGetInstanceProcAddr( instance, "vkCreateMetalSurfaceEXT" ) );
#endif /*VK_USE_PLATFORM_METAL_EXT*/
#ifdef VK_USE_PLATFORM_GGP
      vkCreateStreamDescriptorSurfaceGGP = PFN_vkCreateStreamDescriptorSurfaceGGP( vkGetInstanceProcAddr( instance, "vkCreateStreamDescriptorSurfaceGGP" ) );
#endif /*VK_USE_PLATFORM_GGP*/
#ifdef VK_USE_PLATFORM_VI_NN
      vkCreateViSurfaceNN = PFN_vkCreateViSurfaceNN( vkGetInstanceProcAddr( instance, "vkCreateViSurfaceNN" ) );
#endif /*VK_USE_PLATFORM_VI_NN*/
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
      vkCreateWaylandSurfaceKHR = PFN_vkCreateWaylandSurfaceKHR( vkGetInstanceProcAddr( instance, "vkCreateWaylandSurfaceKHR" ) );
#endif /*VK_USE_PLATFORM_WAYLAND_KHR*/
#ifdef VK_USE_PLATFORM_WIN32_KHR
      vkCreateWin32SurfaceKHR = PFN_vkCreateWin32SurfaceKHR( vkGetInstanceProcAddr( instance, "vkCreateWin32SurfaceKHR" ) );
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
#ifdef VK_USE_PLATFORM_XCB_KHR
      vkCreateXcbSurfaceKHR = PFN_vkCreateXcbSurfaceKHR( vkGetInstanceProcAddr( instance, "vkCreateXcbSurfaceKHR" ) );
#endif /*VK_USE_PLATFORM_XCB_KHR*/
#ifdef VK_USE_PLATFORM_XLIB_KHR
      vkCreateXlibSurfaceKHR = PFN_vkCreateXlibSurfaceKHR( vkGetInstanceProcAddr( instance, "vkCreateXlibSurfaceKHR" ) );
#endif /*VK_USE_PLATFORM_XLIB_KHR*/
      vkDebugReportMessageEXT = PFN_vkDebugReportMessageEXT( vkGetInstanceProcAddr( instance, "vkDebugReportMessageEXT" ) );
      vkDestroyDebugReportCallbackEXT = PFN_vkDestroyDebugReportCallbackEXT( vkGetInstanceProcAddr( instance, "vkDestroyDebugReportCallbackEXT" ) );
      vkDestroyDebugUtilsMessengerEXT = PFN_vkDestroyDebugUtilsMessengerEXT( vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" ) );
      vkDestroyInstance = PFN_vkDestroyInstance( vkGetInstanceProcAddr( instance, "vkDestroyInstance" ) );
      vkDestroySurfaceKHR = PFN_vkDestroySurfaceKHR( vkGetInstanceProcAddr( instance, "vkDestroySurfaceKHR" ) );
      vkEnumeratePhysicalDeviceGroups = PFN_vkEnumeratePhysicalDeviceGroups( vkGetInstanceProcAddr( instance, "vkEnumeratePhysicalDeviceGroups" ) );
      vkEnumeratePhysicalDeviceGroupsKHR = PFN_vkEnumeratePhysicalDeviceGroupsKHR( vkGetInstanceProcAddr( instance, "vkEnumeratePhysicalDeviceGroupsKHR" ) );
      vkEnumeratePhysicalDevices = PFN_vkEnumeratePhysicalDevices( vkGetInstanceProcAddr( instance, "vkEnumeratePhysicalDevices" ) );
      vkSubmitDebugUtilsMessageEXT = PFN_vkSubmitDebugUtilsMessageEXT( vkGetInstanceProcAddr( instance, "vkSubmitDebugUtilsMessageEXT" ) );
#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
      vkAcquireXlibDisplayEXT = PFN_vkAcquireXlibDisplayEXT( vkGetInstanceProcAddr( instance, "vkAcquireXlibDisplayEXT" ) );
#endif /*VK_USE_PLATFORM_XLIB_XRANDR_EXT*/
      vkCreateDevice = PFN_vkCreateDevice( vkGetInstanceProcAddr( instance, "vkCreateDevice" ) );
      vkCreateDisplayModeKHR = PFN_vkCreateDisplayModeKHR( vkGetInstanceProcAddr( instance, "vkCreateDisplayModeKHR" ) );
      vkEnumerateDeviceExtensionProperties = PFN_vkEnumerateDeviceExtensionProperties( vkGetInstanceProcAddr( instance, "vkEnumerateDeviceExtensionProperties" ) );
      vkEnumerateDeviceLayerProperties = PFN_vkEnumerateDeviceLayerProperties( vkGetInstanceProcAddr( instance, "vkEnumerateDeviceLayerProperties" ) );
      vkGetDisplayModeProperties2KHR = PFN_vkGetDisplayModeProperties2KHR( vkGetInstanceProcAddr( instance, "vkGetDisplayModeProperties2KHR" ) );
      vkGetDisplayModePropertiesKHR = PFN_vkGetDisplayModePropertiesKHR( vkGetInstanceProcAddr( instance, "vkGetDisplayModePropertiesKHR" ) );
      vkGetDisplayPlaneCapabilities2KHR = PFN_vkGetDisplayPlaneCapabilities2KHR( vkGetInstanceProcAddr( instance, "vkGetDisplayPlaneCapabilities2KHR" ) );
      vkGetDisplayPlaneCapabilitiesKHR = PFN_vkGetDisplayPlaneCapabilitiesKHR( vkGetInstanceProcAddr( instance, "vkGetDisplayPlaneCapabilitiesKHR" ) );
      vkGetDisplayPlaneSupportedDisplaysKHR = PFN_vkGetDisplayPlaneSupportedDisplaysKHR( vkGetInstanceProcAddr( instance, "vkGetDisplayPlaneSupportedDisplaysKHR" ) );
      vkGetPhysicalDeviceCalibrateableTimeDomainsEXT = PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceCalibrateableTimeDomainsEXT" ) );
#if defined(VK_NV_cooperative_matrix)
      vkGetPhysicalDeviceCooperativeMatrixPropertiesNV = PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceCooperativeMatrixPropertiesNV" ) );
#endif
      vkGetPhysicalDeviceDisplayPlaneProperties2KHR = PFN_vkGetPhysicalDeviceDisplayPlaneProperties2KHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceDisplayPlaneProperties2KHR" ) );
      vkGetPhysicalDeviceDisplayPlanePropertiesKHR = PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceDisplayPlanePropertiesKHR" ) );
      vkGetPhysicalDeviceDisplayProperties2KHR = PFN_vkGetPhysicalDeviceDisplayProperties2KHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceDisplayProperties2KHR" ) );
      vkGetPhysicalDeviceDisplayPropertiesKHR = PFN_vkGetPhysicalDeviceDisplayPropertiesKHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceDisplayPropertiesKHR" ) );
      vkGetPhysicalDeviceExternalBufferProperties = PFN_vkGetPhysicalDeviceExternalBufferProperties( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceExternalBufferProperties" ) );
      vkGetPhysicalDeviceExternalBufferPropertiesKHR = PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceExternalBufferPropertiesKHR" ) );
      vkGetPhysicalDeviceExternalFenceProperties = PFN_vkGetPhysicalDeviceExternalFenceProperties( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceExternalFenceProperties" ) );
      vkGetPhysicalDeviceExternalFencePropertiesKHR = PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceExternalFencePropertiesKHR" ) );
      vkGetPhysicalDeviceExternalImageFormatPropertiesNV = PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceExternalImageFormatPropertiesNV" ) );
      vkGetPhysicalDeviceExternalSemaphoreProperties = PFN_vkGetPhysicalDeviceExternalSemaphoreProperties( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceExternalSemaphoreProperties" ) );
      vkGetPhysicalDeviceExternalSemaphorePropertiesKHR = PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR" ) );
      vkGetPhysicalDeviceFeatures = PFN_vkGetPhysicalDeviceFeatures( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceFeatures" ) );
      vkGetPhysicalDeviceFeatures2 = PFN_vkGetPhysicalDeviceFeatures2( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceFeatures2" ) );
      vkGetPhysicalDeviceFeatures2KHR = PFN_vkGetPhysicalDeviceFeatures2KHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceFeatures2KHR" ) );
      vkGetPhysicalDeviceFormatProperties = PFN_vkGetPhysicalDeviceFormatProperties( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceFormatProperties" ) );
      vkGetPhysicalDeviceFormatProperties2 = PFN_vkGetPhysicalDeviceFormatProperties2( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceFormatProperties2" ) );
      vkGetPhysicalDeviceFormatProperties2KHR = PFN_vkGetPhysicalDeviceFormatProperties2KHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceFormatProperties2KHR" ) );
      vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX = PFN_vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX" ) );
      vkGetPhysicalDeviceImageFormatProperties = PFN_vkGetPhysicalDeviceImageFormatProperties( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceImageFormatProperties" ) );
      vkGetPhysicalDeviceImageFormatProperties2 = PFN_vkGetPhysicalDeviceImageFormatProperties2( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceImageFormatProperties2" ) );
      vkGetPhysicalDeviceImageFormatProperties2KHR = PFN_vkGetPhysicalDeviceImageFormatProperties2KHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceImageFormatProperties2KHR" ) );
      vkGetPhysicalDeviceMemoryProperties = PFN_vkGetPhysicalDeviceMemoryProperties( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceMemoryProperties" ) );
      vkGetPhysicalDeviceMemoryProperties2 = PFN_vkGetPhysicalDeviceMemoryProperties2( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceMemoryProperties2" ) );
      vkGetPhysicalDeviceMemoryProperties2KHR = PFN_vkGetPhysicalDeviceMemoryProperties2KHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceMemoryProperties2KHR" ) );
      vkGetPhysicalDeviceMultisamplePropertiesEXT = PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceMultisamplePropertiesEXT" ) );
      vkGetPhysicalDevicePresentRectanglesKHR = PFN_vkGetPhysicalDevicePresentRectanglesKHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDevicePresentRectanglesKHR" ) );
      vkGetPhysicalDeviceProperties = PFN_vkGetPhysicalDeviceProperties( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceProperties" ) );
      vkGetPhysicalDeviceProperties2 = PFN_vkGetPhysicalDeviceProperties2( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceProperties2" ) );
      vkGetPhysicalDeviceProperties2KHR = PFN_vkGetPhysicalDeviceProperties2KHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceProperties2KHR" ) );
      vkGetPhysicalDeviceQueueFamilyProperties = PFN_vkGetPhysicalDeviceQueueFamilyProperties( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceQueueFamilyProperties" ) );
      vkGetPhysicalDeviceQueueFamilyProperties2 = PFN_vkGetPhysicalDeviceQueueFamilyProperties2( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceQueueFamilyProperties2" ) );
      vkGetPhysicalDeviceQueueFamilyProperties2KHR = PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceQueueFamilyProperties2KHR" ) );
      vkGetPhysicalDeviceSparseImageFormatProperties = PFN_vkGetPhysicalDeviceSparseImageFormatProperties( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSparseImageFormatProperties" ) );
      vkGetPhysicalDeviceSparseImageFormatProperties2 = PFN_vkGetPhysicalDeviceSparseImageFormatProperties2( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSparseImageFormatProperties2" ) );
      vkGetPhysicalDeviceSparseImageFormatProperties2KHR = PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSparseImageFormatProperties2KHR" ) );
      vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV = PFN_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV" ) );
      vkGetPhysicalDeviceSurfaceCapabilities2EXT = PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSurfaceCapabilities2EXT" ) );
      vkGetPhysicalDeviceSurfaceCapabilities2KHR = PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSurfaceCapabilities2KHR" ) );
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR = PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR" ) );
      vkGetPhysicalDeviceSurfaceFormats2KHR = PFN_vkGetPhysicalDeviceSurfaceFormats2KHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSurfaceFormats2KHR" ) );
      vkGetPhysicalDeviceSurfaceFormatsKHR = PFN_vkGetPhysicalDeviceSurfaceFormatsKHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSurfaceFormatsKHR" ) );
#ifdef VK_USE_PLATFORM_WIN32_KHR
      vkGetPhysicalDeviceSurfacePresentModes2EXT = PFN_vkGetPhysicalDeviceSurfacePresentModes2EXT( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSurfacePresentModes2EXT" ) );
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
      vkGetPhysicalDeviceSurfacePresentModesKHR = PFN_vkGetPhysicalDeviceSurfacePresentModesKHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSurfacePresentModesKHR" ) );
      vkGetPhysicalDeviceSurfaceSupportKHR = PFN_vkGetPhysicalDeviceSurfaceSupportKHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceSurfaceSupportKHR" ) );
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
      vkGetPhysicalDeviceWaylandPresentationSupportKHR = PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceWaylandPresentationSupportKHR" ) );
#endif /*VK_USE_PLATFORM_WAYLAND_KHR*/
#ifdef VK_USE_PLATFORM_WIN32_KHR
      vkGetPhysicalDeviceWin32PresentationSupportKHR = PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR" ) );
#endif /*VK_USE_PLATFORM_WIN32_KHR*/
#ifdef VK_USE_PLATFORM_XCB_KHR
      vkGetPhysicalDeviceXcbPresentationSupportKHR = PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceXcbPresentationSupportKHR" ) );
#endif /*VK_USE_PLATFORM_XCB_KHR*/
#ifdef VK_USE_PLATFORM_XLIB_KHR
      vkGetPhysicalDeviceXlibPresentationSupportKHR = PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR( vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceXlibPresentationSupportKHR" ) );
#endif /*VK_USE_PLATFORM_XLIB_KHR*/
#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
      vkGetRandROutputDisplayEXT = PFN_vkGetRandROutputDisplayEXT( vkGetInstanceProcAddr( instance, "vkGetRandROutputDisplayEXT" ) );
#endif /*VK_USE_PLATFORM_XLIB_XRANDR_EXT*/
      vkReleaseDisplayEXT = PFN_vkReleaseDisplayEXT( vkGetInstanceProcAddr( instance, "vkReleaseDisplayEXT" ) );
      vkGetQueueCheckpointDataNV = PFN_vkGetQueueCheckpointDataNV( device ? vkGetDeviceProcAddr( device, "vkGetQueueCheckpointDataNV" ) : vkGetInstanceProcAddr( instance, "vkGetQueueCheckpointDataNV" ) );
      vkQueueBeginDebugUtilsLabelEXT = PFN_vkQueueBeginDebugUtilsLabelEXT( device ? vkGetDeviceProcAddr( device, "vkQueueBeginDebugUtilsLabelEXT" ) : vkGetInstanceProcAddr( instance, "vkQueueBeginDebugUtilsLabelEXT" ) );
      vkQueueBindSparse = PFN_vkQueueBindSparse( device ? vkGetDeviceProcAddr( device, "vkQueueBindSparse" ) : vkGetInstanceProcAddr( instance, "vkQueueBindSparse" ) );
      vkQueueEndDebugUtilsLabelEXT = PFN_vkQueueEndDebugUtilsLabelEXT( device ? vkGetDeviceProcAddr( device, "vkQueueEndDebugUtilsLabelEXT" ) : vkGetInstanceProcAddr( instance, "vkQueueEndDebugUtilsLabelEXT" ) );
      vkQueueInsertDebugUtilsLabelEXT = PFN_vkQueueInsertDebugUtilsLabelEXT( device ? vkGetDeviceProcAddr( device, "vkQueueInsertDebugUtilsLabelEXT" ) : vkGetInstanceProcAddr( instance, "vkQueueInsertDebugUtilsLabelEXT" ) );
      vkQueuePresentKHR = PFN_vkQueuePresentKHR( device ? vkGetDeviceProcAddr( device, "vkQueuePresentKHR" ) : vkGetInstanceProcAddr( instance, "vkQueuePresentKHR" ) );
      vkQueueSubmit = PFN_vkQueueSubmit( device ? vkGetDeviceProcAddr( device, "vkQueueSubmit" ) : vkGetInstanceProcAddr( instance, "vkQueueSubmit" ) );
      vkQueueWaitIdle = PFN_vkQueueWaitIdle( device ? vkGetDeviceProcAddr( device, "vkQueueWaitIdle" ) : vkGetInstanceProcAddr( instance, "vkQueueWaitIdle" ) );
    }
  };
} // namespace WZ_vk
