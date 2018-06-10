#include <string>
#include <vector>

#define VK_USE_PLATFORM_WIN32_KHR
#include "resources\vulkan\vulkan.h"
#include "system.h"
#include "assert.h"

#include "using_vk_mode.setting"
#include "vkbase.h"

bool VKBase::LoadVulkanLibrary()
{
  m_vulkan_library = LoadLibrary("vulkan-1.dll"); //this framework only works in Win32

  if (m_vulkan_library == nullptr)
  {
    assert("Could not load Vulkan library!", "Vulkan", Assert::Error);
    return false;
  }
  return true;
}

bool VKBase::LoadExportedEntryPoints()
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

bool VKBase::LoadGlobalLevelEntryPoints()
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

bool VKBase::CreateInstance()
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
    0, nullptr,
    0, //enabledExtensionCount
    nullptr //enabledExtensionNames
  };

  if (vkCreateInstance(&instance_create_info, nullptr, &m_instance) != VK_SUCCESS)
  {
    assert("Could not create Vulkan instance!", "Vulkan", Assert::Error);
    return false;
  }

  return true;
}

bool VKBase::LoadInstanceLevelEntryPoints()
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

bool VKBase::CheckPhysicalDeviceProperties(VkPhysicalDevice physical_device, uint32_t &queue_family_index)
{
  VkPhysicalDeviceProperties device_properties;
  VkPhysicalDeviceFeatures   device_features;

  vkGetPhysicalDeviceProperties(physical_device, &device_properties);
  vkGetPhysicalDeviceFeatures(physical_device, &device_features);

  uint32_t major_version = VK_VERSION_MAJOR(device_properties.apiVersion);
  uint32_t minor_version = VK_VERSION_MINOR(device_properties.apiVersion);
  uint32_t patch_version = VK_VERSION_PATCH(device_properties.apiVersion);

  if ((major_version < 1) || (device_properties.limits.maxImageDimension2D < 4096))
    return false;

  uint32_t queue_families_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, nullptr);
  if (queue_families_count == 0)
    return false;

  std::vector<VkQueueFamilyProperties> queue_family_properties(queue_families_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, queue_family_properties.data());
  for (uint32_t i = 0; i < queue_families_count; ++i)
    if ((queue_family_properties[i].queueCount > 0) && (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
    {
      queue_family_index = i;
      return true;
    }

  OutputDebugString(("Could not find queue family with required properties on physical device " +std::string(device_properties.deviceName) + "!\n").c_str());
  return false;
}

bool VKBase::CreateDevice()
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

  uint32_t selected_queue_family_index = UINT32_MAX;
  for (uint32_t i = 0; i < num_devices; ++i)
    if (CheckPhysicalDeviceProperties(physical_devices[i], selected_queue_family_index))
    {
      m_physical_device = physical_devices[i];
      break;
    }

  if (m_physical_device == VK_NULL_HANDLE)
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
    &queue_create_info, //arrays of queues goes into here
    0, nullptr, 0, nullptr, nullptr                     
  };

  if (vkCreateDevice(m_physical_device, &device_create_info, nullptr, &m_device) != VK_SUCCESS)
  {
    assert("Could not create Vulkan device!", "Vulkan", Assert::Error);
    return false;
  }

  //m_queue_family_index = selected_queue_family_index;
  return true;
}

bool VKBase::LoadDeviceLevelEntryPoints()
{
#define LOAD_DEVICE_LEVEL( func )                                                 \
if( !(func = (PFN_##func)vkGetDeviceProcAddr( m_device, #func )) ) {              \
assert("Could not load device level function: "#func, "Vulkan", Assert::Error);   \
return false;                                                                     \
}
#include "vulkanFunctions.inl"
#undef LOAD_DEVICE_LEVEL

  return true;
}

bool VKBase::GetDeviceQueue()
{
  //no queues were created in base!

  return true;
}

bool VKBase::Initialize()
{
#define CHECK(x) if (!x()) return false;

  CHECK(LoadVulkanLibrary)
    CHECK(LoadExportedEntryPoints)
    CHECK(LoadGlobalLevelEntryPoints)
    CHECK(CreateInstance)
    CHECK(LoadInstanceLevelEntryPoints)
    CHECK(CreateDevice)
    CHECK(LoadDeviceLevelEntryPoints)

#undef Run

    return true;
}

void VKBase::Update()
{
  //Do nothing here
}

void VKBase::Terminate()
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