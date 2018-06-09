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
//LOAD_INSTANCE_LEVEL(vkEnumerateDeviceExtensionProperties)
LOAD_INSTANCE_LEVEL(vkCreateWin32SurfaceKHR)

#undef LOAD_INSTANCE_LEVEL

/////////////////////////////////////////////////////////////////////////////////

#ifndef LOAD_DEVICE_LEVEL
#define LOAD_DEVICE_LEVEL(func)
#endif

LOAD_DEVICE_LEVEL(vkGetDeviceQueue)
LOAD_DEVICE_LEVEL(vkDestroyDevice)
LOAD_DEVICE_LEVEL(vkDeviceWaitIdle)

#undef LOAD_DEVICE_LEVEL