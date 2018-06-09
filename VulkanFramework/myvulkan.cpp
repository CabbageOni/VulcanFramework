#include <Windows.h>
#include <string>
#include <iostream>
#include <vector>

#include "myvulkan.h"
#include "assert.h"
#include "system.h"

//current tutorial: https://software.intel.com/en-us/articles/api-without-secrets-introduction-to-vulkan-part-2

Vulkan vulkan;

bool Vulkan::LoadVulkanLibrary()
{
  m_vulkan_library = LoadLibrary("vulkan-1.dll"); //this framework only works in Win32

  if (m_vulkan_library == nullptr)
  {
    assert("Could not load Vulkan library!", "Vulkan", Assert::Error);
    return false;
  }
  return true;
}

bool Vulkan::LoadExportedEntryPoints()
{
#define LOAD_EXPORTED( func )                                                     \
    if( !(func = (PFN_##func)GetProcAddress( m_vulkan_library, #func )) ) {       \
      assert("Could not load exported function: "#func, "Vulkan", Assert::Error); \
      return false;                                                               \
    }
#include "vulkanFunctions.inl"
#undef LOAD_EXPORTED

  return true;
}

bool Vulkan::LoadGlobalLevelEntryPoints()
{
#define LOAD_GLOBAL_LEVEL( func )                                                     \
    if( !(func = (PFN_##func)vkGetInstanceProcAddr( nullptr, #func )) ) {             \
      assert("Could not load global level function: "#func, "Vulkan", Assert::Error); \
      return false;                                                                   \
    }
#include "vulkanFunctions.inl"
#undef LOAD_GLOBAL_LEVEL

  return true;
}

bool Vulkan::CreateInstance()
{
  uint32_t extensions_count = 0;
  if ((vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, nullptr) != VK_SUCCESS) || (extensions_count == 0))
  {
    assert("Error occurred during instance extensions enumeration!", "Vulkan", Assert::Error);
    return false;
  }
  OutputDebugString(("number of extensions: " + std::to_string(extensions_count) + "\n").c_str());

  std::vector<VkExtensionProperties> available_extensions(extensions_count);
  if (vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, available_extensions.data()) != VK_SUCCESS)
  {
    assert("Error occurred during instance extensions enumeration!", "Vulkan", Assert::Error);
    return false;
  }

  std::vector<const char*> extensions =
  {
    VK_KHR_SURFACE_EXTENSION_NAME, //displaying window extension
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME, //exclusive for Windows OS
  };

  //for each extensions search for specific extension
  for (size_t i = 0; i < extensions.size(); ++i)
    if (!CheckExtensionAvailability(extensions[i], available_extensions))
    {
      assert(("Could not find instance extension named \"" + std::string(extensions[i]) + "\"!").c_str(), "Vulkan", Assert::Error);
      return false;
    }

  VkApplicationInfo application_info = { //peek VkApplicationInfo for more details
    VK_STRUCTURE_TYPE_APPLICATION_INFO,             
    nullptr,                                        
    "Vulkan Framework",                             
    VK_MAKE_VERSION(1, 0, 0),                       
    "Vulkan Framework",                             
    VK_MAKE_VERSION(1, 0, 0),
    VK_API_VERSION_1_0
  };

  VkInstanceCreateInfo instance_create_info = { //peek VkInstanceCreateInfo for more details
    VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    nullptr, 0,                                       
    &application_info,                       
    0, nullptr,
    static_cast<uint32_t>(extensions.size()), //enabledExtensionCount
    (extensions.empty() ? nullptr : &extensions[0]) //enabledExtensionNames
  };

  if (vkCreateInstance(&instance_create_info, nullptr, &m_instance) != VK_SUCCESS)
  {
    assert("Could not create Vulkan instance!", "Vulkan", Assert::Error);
    return false;
  }

  return true;
}

bool Vulkan::CheckExtensionAvailability(const char* extension_name, const std::vector<VkExtensionProperties>& available_extensions)
{
  for (size_t i = 0; i < available_extensions.size(); ++i)
    if (strcmp(available_extensions[i].extensionName, extension_name) == 0)
      return true;
  return false;
}

bool Vulkan::LoadInstanceLevelEntryPoints()
{
#define LOAD_INSTANCE_LEVEL( func )                                                     \
    if( !(func = (PFN_##func)vkGetInstanceProcAddr( m_instance, #func )) ) {            \
      assert("Could not load instance level function: "#func, "Vulkan", Assert::Error); \
      return false;                                                                     \
    }
#include "vulkanFunctions.inl"
#undef LOAD_INSTANCE_LEVEL

  return true;
}

bool Vulkan::CreateDevice()
{
  uint32_t num_devices = 0;
  if ((vkEnumeratePhysicalDevices(m_instance, &num_devices, nullptr) != VK_SUCCESS) || (num_devices == 0))
  {
    assert("Error occurred during physical devices enumeration!", "Vulkan", Assert::Error);
    return false;
  }

  OutputDebugString(("number of physical devices: " + std::to_string(num_devices) + "\n").c_str());

  std::vector<VkPhysicalDevice> physical_devices(num_devices);
  if (vkEnumeratePhysicalDevices(m_instance, &num_devices, &physical_devices[0]) != VK_SUCCESS)
  {
    assert("Error occurred during physical devices enumeration!", "Vulkan", Assert::Error);
    return false;
  }

  uint32_t selected_graphics_queue_family_index = UINT32_MAX;
  uint32_t selected_present_queue_family_index = UINT32_MAX;

  for (uint32_t i = 0; i < num_devices; ++i)
    if (CheckPhysicalDeviceProperties(physical_devices[i], selected_graphics_queue_family_index, selected_present_queue_family_index))
    {
      m_physical_device = physical_devices[i];
      break;
    }

  if (m_physical_device == VK_NULL_HANDLE)
  {
    assert("Could not select physical device based on the chosen properties!", "Vulkan", Assert::Error);
    return false;
  }

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  std::vector<float> queue_priorities = { 1.0f }; //higher float = given more time to compute, but not always garunteed
                                                  //also only per device, independent from other devices
  
  queue_create_infos.push_back( { //peek VkDeviceQueueCreateInfo for more details
    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,    
    nullptr, 0,                                             
    selected_graphics_queue_family_index,
    static_cast<uint32_t>(queue_priorities.size()),
    queue_priorities.data()                        
  });
  
  //if graphics and present uses different queue family
  if (selected_graphics_queue_family_index != selected_present_queue_family_index)
  {
    queue_create_infos.push_back({ //peek VkDeviceQueueCreateInfo for more details
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      nullptr, 0,
      selected_present_queue_family_index,
      static_cast<uint32_t>(queue_priorities.size()),
      queue_priorities.data()
    });
  }

  std::vector<const char*> extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

  VkDeviceCreateInfo device_create_info = { //peek VkDeviceCreateInfo for more details
    VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    nullptr, 0,                                   
    static_cast<uint32_t>(queue_create_infos.size()), //num of queue families                                 
    queue_create_infos.data(),
    0, nullptr,
    static_cast<uint32_t>(extensions.size()),
    extensions.data(),
    nullptr // pointer to device features                              
  };
  
  if (vkCreateDevice(m_physical_device, &device_create_info, nullptr, &m_device) != VK_SUCCESS)
  {
    assert("Could not create Vulkan device!", "Vulkan", Assert::Error);
    return false;
  }
  
  m_graphics_queue_family_index = selected_graphics_queue_family_index;
  m_present_queue_family_index = selected_present_queue_family_index;
  return true;
}

bool Vulkan::CheckPhysicalDeviceProperties(VkPhysicalDevice physical_device,
                                          uint32_t& selected_graphics_queue_family_index,
                                          uint32_t &selected_present_queue_family_index)
{
  VkPhysicalDeviceProperties device_properties;
  VkPhysicalDeviceFeatures   device_features;

  vkGetPhysicalDeviceProperties(physical_device, &device_properties);
  vkGetPhysicalDeviceFeatures(physical_device, &device_features);

  uint32_t extensions_count = 0;
  if ((vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensions_count, nullptr) != VK_SUCCESS) || (extensions_count == 0))
  {
    OutputDebugString(("Error occurred during physical device \"" + std::string(device_properties.deviceName) + "\" extensions enumeration!\n").c_str());
    return false;
  }

  std::vector<VkExtensionProperties> available_extensions(extensions_count);
  if (vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensions_count, available_extensions.data()) != VK_SUCCESS)
  {
    OutputDebugString(("Error occurred during physical device \"" + std::string(device_properties.deviceName) + "\" extensions enumeration!\n").c_str());
    return false;
  }

  std::vector<const char*> device_extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

  for (size_t i = 0; i < device_extensions.size(); ++i)
    if (!CheckExtensionAvailability(device_extensions[i], available_extensions))
    {
      OutputDebugString(("Physical device \"" + std::string(device_properties.deviceName) + "\" doesn't support extension named\"" + std::string(device_extensions[i]) + "\"!").c_str());
      return false;
    }

  uint32_t major_version = VK_VERSION_MAJOR(device_properties.apiVersion);
  //  uint32_t minor_version = VK_VERSION_MINOR(device_properties.apiVersion);
  //  uint32_t patch_version = VK_VERSION_PATCH(device_properties.apiVersion);
  
  if ((major_version < 1) || (device_properties.limits.maxImageDimension2D < 4096))
  {
    OutputDebugString(("Physical device \"" + std::string(device_properties.deviceName) + "\" doesn't support required parameters!").c_str());
    return false;
  }

  uint32_t queue_families_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, nullptr);
  if (queue_families_count == 0)
  {
    OutputDebugString(("Physical device " + std::string(device_properties.deviceName) + " doesn't have any queue families!\n").c_str());
    return false;
  }
  
  std::vector<VkQueueFamilyProperties>  queue_family_properties(queue_families_count);
  std::vector<VkBool32> queue_present_support(queue_families_count);
  
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, queue_family_properties.data());

  uint32_t graphics_queue_family_index = UINT32_MAX;
  uint32_t present_queue_family_index = UINT32_MAX;
  
  for (uint32_t i = 0; i < queue_families_count; ++i)
  {
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, m_presentation_surface, &queue_present_support[i]);

    if ((queue_family_properties[i].queueCount > 0) && (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
    {
      // Select first queue that supports graphics
      if (graphics_queue_family_index == UINT32_MAX)
      {
        graphics_queue_family_index = i;
      }
  
      // If there is queue that supports both graphics and present - prefer it
      if (queue_present_support[i])
      {
        selected_graphics_queue_family_index = i;
        selected_present_queue_family_index = i;
        OutputDebugString(("Selected device: " + std::string(device_properties.deviceName) + "\n").c_str());
        return true;
      }
    }
  }

  // We don't have queue that supports both graphics and present so we have to use separate queues
  for (uint32_t i = 0; i < queue_families_count; ++i)
  {
    if (queue_present_support[i])
    {
      present_queue_family_index = i;
      break;
    }
  }

  // If this device doesn't support queues with graphics and present capabilities don't use it
  if ((graphics_queue_family_index == UINT32_MAX) || (present_queue_family_index == UINT32_MAX))
  {
    OutputDebugString(("Could not find queue family with required properties on physical device \"" + std::string(device_properties.deviceName) + "\"!\n").c_str());
    return false;
  }

  selected_graphics_queue_family_index = graphics_queue_family_index;
  selected_present_queue_family_index = present_queue_family_index;
  OutputDebugString(("Selected device: " + std::string(device_properties.deviceName) + "\n").c_str());
  return true;
}

bool Vulkan::LoadDeviceLevelEntryPoints()
{
#define LOAD_DEVICE_LEVEL( func )                                                       \
    if( !(func = (PFN_##func)vkGetDeviceProcAddr( m_device, #func )) ) {                \
      assert("Could not load device level function: "#func, "Vulkan", Assert::Error);   \
      return false;                                                                     \
    }
#include "vulkanFunctions.inl"
#undef LOAD_DEVICE_LEVEL

  return true;
}

bool Vulkan::GetDeviceQueue()
{
  vkGetDeviceQueue(m_device, m_graphics_queue_family_index, 0 /*queue_index*/, &m_graphics_queue); //need to call this per loading queue!
  vkGetDeviceQueue(m_device, m_present_queue_family_index, 0, &m_present_queue);
  return true;
}

bool Vulkan::CreatePresentationSurface()
{
  VkWin32SurfaceCreateInfoKHR surface_create_info = { //peek VkWin32SurfaceCreateInfoKHR for more info
    VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
    nullptr, 0,
     winAPI.InstanceHandle(), // HINSTANCE hinstance
     winAPI.WindowHandle() // HWND hwnd
  };

  if (vkCreateWin32SurfaceKHR(m_instance, &surface_create_info, nullptr, &m_presentation_surface) == VK_SUCCESS)
    return true;

  assert("Could not create presentation surface!", "Vulkan", Assert::Error);
  return false;
}

bool Vulkan::CreateSemaphores()
{
  VkSemaphoreCreateInfo semaphore_create_info = { //peek VkSemaphoreCreateInfo for details
    VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    nullptr, 0 };

  if ((vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &m_image_available_semaphore) != VK_SUCCESS) ||
    (vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &m_rendering_finished_semaphore) != VK_SUCCESS))
  {
    assert("Could not create semaphores!", "Vulkan", Assert::Error);
    return false;
  }

  return true;
}

bool Vulkan::CreateSwapChain()
{
  VkSurfaceCapabilitiesKHR surface_capabilities;
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_presentation_surface, &surface_capabilities) != VK_SUCCESS)
  {
    assert("Could not check presentation surface capabilities!", "Vulkan", Assert::Error);
    return false;
  }

  uint32_t formats_count;
  if ((vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_presentation_surface, &formats_count, nullptr) != VK_SUCCESS) || (formats_count == 0))
  {
    assert("Error occurred during presentation surface formats enumeration!", "Vulkan", Assert::Error);
    return false;
  }

  std::vector<VkSurfaceFormatKHR> surface_formats(formats_count);
  if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_presentation_surface, &formats_count, surface_formats.data()) != VK_SUCCESS)
  {
    assert("Error occurred during presentation surface formats enumeration!", "Vulkan", Assert::Error);
    return false;
  }

  return true;
}

bool Vulkan::Initialize()
{
#define CHECK(x) if (!x()) return false;

  CHECK(LoadVulkanLibrary)
  CHECK(LoadExportedEntryPoints)
  CHECK(LoadGlobalLevelEntryPoints)
  CHECK(CreateInstance)
  CHECK(LoadInstanceLevelEntryPoints)
  CHECK(CreatePresentationSurface)
  CHECK(CreateDevice)
  CHECK(LoadDeviceLevelEntryPoints)
  CHECK(GetDeviceQueue)
  CHECK(CreateSemaphores)

#undef Run

  return true;
}

void Vulkan::Update()
{
  //currently doing nothing here
}

void Vulkan::Terminate()
{
  //warning! all releasing should be in order!

  if (m_device != VK_NULL_HANDLE)
  {
    vkDeviceWaitIdle(m_device);
    vkDestroyDevice(m_device, nullptr);
  }

  if (m_instance != VK_NULL_HANDLE)
    vkDestroyInstance(m_instance, nullptr);

  if (m_vulkan_library)
    FreeLibrary(m_vulkan_library);
}