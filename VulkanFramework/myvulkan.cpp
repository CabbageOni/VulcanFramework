#include <Windows.h>
#include <string>
#include <iostream>
#include <vector>

#include "myvulkan.h"
#include "assert.h"

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

  OutputDebugString(("number of actual using physical devices: " + std::to_string(num_devices) + "\n").c_str());

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

bool Vulkan::Initialize()
{
#define CHECK(x) if (!x()) return false;

  CHECK(LoadVulkanLibrary)
  CHECK(LoadExportedEntryPoints)
  CHECK(LoadGlobalLevelEntryPoints)
  CHECK(CreateInstance)
  CHECK(LoadInstanceLevelEntryPoints)
  CHECK(CreateDevice)

#undef Run

  return true;
}