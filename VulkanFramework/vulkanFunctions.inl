#ifndef LOAD_EXPORTED
#define LOAD_EXPORTED(func)
#endif

LOAD_EXPORTED(vkGetInstanceProcAddr)

#undef LOAD_EXPORTED

/////////////////////////////////////////////////////////////////////////////////

#ifndef LOAD_GLOBAL_LEVEL
#define LOAD_GLOBAL_LEVEL(func)
#endif

LOAD_GLOBAL_LEVEL(vkCreateInstance)
LOAD_GLOBAL_LEVEL(vkEnumerateInstanceExtensionProperties)
//LOAD_GLOBAL_LEVEL(vkEnumerateInstanceLayerProperties)

#undef LOAD_GLOBAL_LEVEL

/////////////////////////////////////////////////////////////////////////////////

#ifndef LOAD_INSTANCE_LEVEL
#define LOAD_INSTANCE_LEVEL(func)
#endif

LOAD_INSTANCE_LEVEL(vkDestroyInstance)
LOAD_INSTANCE_LEVEL(vkEnumeratePhysicalDevices)
LOAD_INSTANCE_LEVEL(vkGetPhysicalDeviceProperties)
LOAD_INSTANCE_LEVEL(vkGetPhysicalDeviceFeatures)
LOAD_INSTANCE_LEVEL(vkGetPhysicalDeviceQueueFamilyProperties)
LOAD_INSTANCE_LEVEL(vkCreateDevice)
LOAD_INSTANCE_LEVEL(vkGetDeviceProcAddr)
LOAD_INSTANCE_LEVEL(vkEnumerateDeviceExtensionProperties)

// load surface related (presenting)
LOAD_INSTANCE_LEVEL(vkCreateWin32SurfaceKHR)
LOAD_INSTANCE_LEVEL(vkDestroySurfaceKHR)
LOAD_INSTANCE_LEVEL(vkGetPhysicalDeviceSurfaceSupportKHR)
LOAD_INSTANCE_LEVEL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
LOAD_INSTANCE_LEVEL(vkGetPhysicalDeviceSurfaceFormatsKHR)
LOAD_INSTANCE_LEVEL(vkGetPhysicalDeviceSurfacePresentModesKHR)

#if VK_CURRENT_MODE == VK_VERTEX_ATTRIBUTES
LOAD_INSTANCE_LEVEL(vkGetPhysicalDeviceMemoryProperties)
#endif

#undef LOAD_INSTANCE_LEVEL

/////////////////////////////////////////////////////////////////////////////////

#ifndef LOAD_DEVICE_LEVEL
#define LOAD_DEVICE_LEVEL(func)
#endif

LOAD_DEVICE_LEVEL(vkGetDeviceQueue)
LOAD_DEVICE_LEVEL(vkDestroyDevice)
LOAD_DEVICE_LEVEL(vkDeviceWaitIdle)
LOAD_DEVICE_LEVEL(vkCreateSemaphore)
LOAD_DEVICE_LEVEL(vkDestroySemaphore)
LOAD_DEVICE_LEVEL(vkQueueSubmit)
LOAD_DEVICE_LEVEL(vkCreateCommandPool)
LOAD_DEVICE_LEVEL(vkDestroyCommandPool)
LOAD_DEVICE_LEVEL(vkAllocateCommandBuffers)
LOAD_DEVICE_LEVEL(vkBeginCommandBuffer)
LOAD_DEVICE_LEVEL(vkEndCommandBuffer)
LOAD_DEVICE_LEVEL(vkFreeCommandBuffers)
LOAD_DEVICE_LEVEL(vkCmdPipelineBarrier)
LOAD_DEVICE_LEVEL(vkCmdClearColorImage)

#if VK_CURRENT_MODE == VK_SWAPCHAIN
LOAD_DEVICE_LEVEL(vkCreateSwapchainKHR)
LOAD_DEVICE_LEVEL(vkDestroySwapchainKHR)
LOAD_DEVICE_LEVEL(vkGetSwapchainImagesKHR)
LOAD_DEVICE_LEVEL(vkAcquireNextImageKHR)
LOAD_DEVICE_LEVEL(vkQueuePresentKHR)
#elif VK_CURRENT_MODE == VK_FIRST_TRIANGLE
LOAD_DEVICE_LEVEL(vkCreateSwapchainKHR)
LOAD_DEVICE_LEVEL(vkDestroySwapchainKHR)
LOAD_DEVICE_LEVEL(vkGetSwapchainImagesKHR)
LOAD_DEVICE_LEVEL(vkAcquireNextImageKHR)
LOAD_DEVICE_LEVEL(vkQueuePresentKHR)
LOAD_DEVICE_LEVEL(vkCreateRenderPass)
LOAD_DEVICE_LEVEL(vkDestroyRenderPass)
LOAD_DEVICE_LEVEL(vkCreateImageView)
LOAD_DEVICE_LEVEL(vkDestroyImageView)
LOAD_DEVICE_LEVEL(vkCreateFramebuffer)
LOAD_DEVICE_LEVEL(vkDestroyFramebuffer)
LOAD_DEVICE_LEVEL(vkCreateShaderModule)
LOAD_DEVICE_LEVEL(vkDestroyShaderModule)
LOAD_DEVICE_LEVEL(vkCreatePipelineLayout)
LOAD_DEVICE_LEVEL(vkDestroyPipelineLayout)
LOAD_DEVICE_LEVEL(vkCreateGraphicsPipelines)
LOAD_DEVICE_LEVEL(vkDestroyPipeline)
LOAD_DEVICE_LEVEL(vkCmdBeginRenderPass)
LOAD_DEVICE_LEVEL(vkCmdBindPipeline)
LOAD_DEVICE_LEVEL(vkCmdDraw)
LOAD_DEVICE_LEVEL(vkCmdEndRenderPass)
#elif VK_CURRENT_MODE == VK_VERTEX_ATTRIBUTES
LOAD_DEVICE_LEVEL(vkCreateSwapchainKHR)
LOAD_DEVICE_LEVEL(vkDestroySwapchainKHR)
LOAD_DEVICE_LEVEL(vkGetSwapchainImagesKHR)
LOAD_DEVICE_LEVEL(vkAcquireNextImageKHR)
LOAD_DEVICE_LEVEL(vkQueuePresentKHR)
LOAD_DEVICE_LEVEL(vkCreateImageView)
LOAD_DEVICE_LEVEL(vkDestroyImageView)
LOAD_DEVICE_LEVEL(vkCreateRenderPass)
LOAD_DEVICE_LEVEL(vkDestroyRenderPass)
LOAD_DEVICE_LEVEL(vkCreateShaderModule)
LOAD_DEVICE_LEVEL(vkDestroyShaderModule)
LOAD_DEVICE_LEVEL(vkCreatePipelineLayout)
LOAD_DEVICE_LEVEL(vkDestroyPipelineLayout)
LOAD_DEVICE_LEVEL(vkCreateGraphicsPipelines)
LOAD_DEVICE_LEVEL(vkDestroyPipeline)
LOAD_DEVICE_LEVEL(vkCreateBuffer)
LOAD_DEVICE_LEVEL(vkDestroyBuffer)
LOAD_DEVICE_LEVEL(vkGetBufferMemoryRequirements)
LOAD_DEVICE_LEVEL(vkAllocateMemory)
LOAD_DEVICE_LEVEL(vkBindBufferMemory)
LOAD_DEVICE_LEVEL(vkMapMemory)
LOAD_DEVICE_LEVEL(vkFlushMappedMemoryRanges)
LOAD_DEVICE_LEVEL(vkUnmapMemory)
LOAD_DEVICE_LEVEL(vkFreeMemory)
LOAD_DEVICE_LEVEL(vkCreateFence)
LOAD_DEVICE_LEVEL(vkWaitForFences)
LOAD_DEVICE_LEVEL(vkResetFences)
LOAD_DEVICE_LEVEL(vkDestroyFence)
LOAD_DEVICE_LEVEL(vkCreateFramebuffer)
LOAD_DEVICE_LEVEL(vkDestroyFramebuffer)
LOAD_DEVICE_LEVEL(vkCmdBeginRenderPass)
LOAD_DEVICE_LEVEL(vkCmdBindPipeline)
LOAD_DEVICE_LEVEL(vkCmdBindVertexBuffers)
LOAD_DEVICE_LEVEL(vkCmdDraw)
LOAD_DEVICE_LEVEL(vkCmdEndRenderPass)
LOAD_DEVICE_LEVEL(vkCmdSetViewport)
LOAD_DEVICE_LEVEL(vkCmdSetScissor)
LOAD_DEVICE_LEVEL(vkCmdCopyBuffer)
#endif

#undef LOAD_DEVICE_LEVEL