#include <Windows.h>
#include <string>
#include <iostream>
#include <vector>

#include "myvulkan.h"
#include "assert.h"

//current tutorial: https://software.intel.com/en-us/articles/api-without-secrets-introduction-to-vulkan-part-2

Vulkan vulkan;

bool Vulkan::LoadVulkanLibrary()
{
  m_vulkanLibrary = LoadLibrary("vulkan-1.dll"); //this framework only works in Win32

  if (m_vulkanLibrary == nullptr)
  {
    assert("Could not load Vulkan library!", "Vulkan", Assert::Error);
    return false;
  }
  return true;
}

bool Vulkan::LoadExportedEntryPoints()
{
#define LOAD_EXPORTED( func )                                                     \
    if( !(func = (PFN_##func)GetProcAddress( m_vulkanLibrary, #func )) ) {        \
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
    0, nullptr, 0, nullptr                                  
  };

  if (vkCreateInstance(&instance_create_info, nullptr, &m_instance) != VK_SUCCESS)
    assert("Could not create Vulkan instance!", "Vulkan", Assert::Error);

  return true;
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

  std::vector<VkPhysicalDevice> physical_devices(num_devices);
  if (vkEnumeratePhysicalDevices(m_instance, &num_devices, &physical_devices[0]) != VK_SUCCESS)
  {
    assert("Error occurred during physical devices enumeration!", "Vulkan", Assert::Error);
    return false;
  }

  OutputDebugString(("number of physical devices: " + std::to_string(num_devices) + "\n").c_str());

  VkPhysicalDevice selected_physical_device = VK_NULL_HANDLE;
  uint32_t selected_queue_family_index = UINT32_MAX;
  for (uint32_t i = 0; i < num_devices; ++i)
    if (CheckPhysicalDeviceProperties(physical_devices[i], selected_queue_family_index))
    {
      selected_physical_device = physical_devices[i];
      break;
    }

  if (selected_physical_device == VK_NULL_HANDLE)
  {
    assert("Could not select physical device based on the chosen properties!", "Vulkan", Assert::Error);
    return false;
  }

  std::vector<float> queue_priorities = { 1.0f }; //higher float = given more time to compute, but not always garunteed
                                                  //also only per device, independent from other devices

  VkDeviceQueueCreateInfo queue_create_info = { //peek VkDeviceQueueCreateInfo for more details
    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,    
    nullptr, 0,                                             
    selected_queue_family_index,                   
    static_cast<uint32_t>(queue_priorities.size()),
    queue_priorities.data()                        
  };

  VkDeviceCreateInfo device_create_info = { //peek VkDeviceCreateInfo for more details
    VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    nullptr, 0,                                   
    1, //num of queue families                                 
    &queue_create_info,           
    0, nullptr, 0, nullptr,
    nullptr // pointer to device features                              
  };

  if (vkCreateDevice(selected_physical_device, &device_create_info, nullptr, &m_device) != VK_SUCCESS)
  {
    assert("Could not create Vulkan device!", "Vulkan", Assert::Error);
    return false;
  }

  m_queue_family_index = selected_queue_family_index;
  return true;
}

bool Vulkan::CheckPhysicalDeviceProperties(VkPhysicalDevice physical_device, uint32_t& queue_family_index)
{
  VkPhysicalDeviceProperties device_properties;
  //VkPhysicalDeviceFeatures device_features;

  vkGetPhysicalDeviceProperties(physical_device, &device_properties);
  //vkGetPhysicalDeviceFeatures(physical_device, &device_features);

  uint32_t major_version = VK_VERSION_MAJOR(device_properties.apiVersion);
  uint32_t minor_version = VK_VERSION_MINOR(device_properties.apiVersion);
  uint32_t patch_version = VK_VERSION_PATCH(device_properties.apiVersion);

  //checking minimum required parameters
  if ((major_version < 1) && (device_properties.limits.maxImageDimension2D < 4096))
  {
    OutputDebugString(("Physical device "+ std::string(device_properties.deviceName) + " doesn't support required parameters!\n").c_str());
    return false;
  }

  //checking queue family for device
  uint32_t queue_families_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, nullptr);
  if (queue_families_count == 0)
  {
    OutputDebugString(("Physical device " + std::string(device_properties.deviceName) + " doesn't have any queue families!\n").c_str());
    return false;
  }

  //load queue family properties
  std::vector<VkQueueFamilyProperties> queue_family_properties(queue_families_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, queue_family_properties.data());
  for (uint32_t i = 0; i < queue_families_count; ++i)
    if ((queue_family_properties[i].queueCount > 0) && (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
    {
      OutputDebugString(("Selected device : " + std::string(device_properties.deviceName) + "\n").c_str());
      queue_family_index = i;
      return true;
    }

  OutputDebugString(("Could not find queue family with required properties on physical device"
    + std::string(device_properties.deviceName) + "!\n").c_str());
  return false;
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
  vkGetDeviceQueue(m_device, m_queue_family_index, 0 /*queue_index*/, &m_queue); //need to call this per loading queue!
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
  CHECK(CreateDevice)
  CHECK(LoadDeviceLevelEntryPoints)
  CHECK(GetDeviceQueue)

#undef Run

  return true;
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

  if (m_vulkanLibrary) 
    FreeLibrary(m_vulkanLibrary);
}