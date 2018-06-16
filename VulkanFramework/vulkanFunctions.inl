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

#if VK_CURRENT_MODE == VK_SWAPCHAIN || VK_CURRENT_MODE == VK_FIRST_TRIANGLE
LOAD_INSTANCE_LEVEL(vkCreateWin32SurfaceKHR)
LOAD_INSTANCE_LEVEL(vkDestroySurfaceKHR)
LOAD_INSTANCE_LEVEL(vkGetPhysicalDeviceSurfaceSupportKHR)
LOAD_INSTANCE_LEVEL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
LOAD_INSTANCE_LEVEL(vkGetPhysicalDeviceSurfaceFormatsKHR)
LOAD_INSTANCE_LEVEL(vkGetPhysicalDeviceSurfacePresentModesKHR)
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
#endif

#undef LOAD_DEVICE_LEVEL