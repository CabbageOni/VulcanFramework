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
  //CanRender = false;
  //
  //if (Vulkan.Device != VK_NULL_HANDLE) {
  //  vkDeviceWaitIdle(Vulkan.Device);
  //}

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

  uint32_t present_modes_count;
  if ((vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device,m_presentation_surface, &present_modes_count, nullptr) != VK_SUCCESS) ||
    (present_modes_count == 0))
  {
    assert("Error occurred during presentation surface present modes enumeration!", "Vulkan", Assert::Error);
    return false;
  }

  std::vector<VkPresentModeKHR> present_modes(present_modes_count);
  if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_presentation_surface, &present_modes_count, present_modes.data()) != VK_SUCCESS)
  {
    assert("Error occurred during presentation surface present modes enumeration!", "Vulkan", Assert::Error);
    return false;
  }

  //uint32_t                      desired_number_of_images = GetSwapChainNumImages(surface_capabilities);
  //VkSurfaceFormatKHR            desired_format = GetSwapChainFormat(surface_formats);
  //VkExtent2D                    desired_extent = GetSwapChainExtent(surface_capabilities);
  //VkImageUsageFlags             desired_usage = GetSwapChainUsageFlags(surface_capabilities);
  //VkSurfaceTransformFlagBitsKHR desired_transform = GetSwapChainTransform(surface_capabilities);
  //VkPresentModeKHR              desired_present_mode = GetSwapChainPresentMode(present_modes);
  //VkSwapchainKHR                old_swap_chain = Vulkan.SwapChain;
  //
  //if (static_cast<int>(desired_usage) == -1) {
  //  return false;
  //}
  //if (static_cast<int>(desired_present_mode) == -1) {
  //  return false;
  //}
  //if ((desired_extent.width == 0) || (desired_extent.height == 0)) {
  //  // Current surface size is (0, 0) so we can't create a swap chain and render anything (CanRender == false)
  //  // But we don't wont to kill the application as this situation may occur i.e. when window gets minimized
  //  return true;
  //}
  //
  //VkSwapchainCreateInfoKHR swap_chain_create_info = {
  //  VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,  // VkStructureType                sType
  //  nullptr,                                      // const void                    *pNext
  //  0,                                            // VkSwapchainCreateFlagsKHR      flags
  //  Vulkan.PresentationSurface,                   // VkSurfaceKHR                   surface
  //  desired_number_of_images,                     // uint32_t                       minImageCount
  //  desired_format.format,                        // VkFormat                       imageFormat
  //  desired_format.colorSpace,                    // VkColorSpaceKHR                imageColorSpace
  //  desired_extent,                               // VkExtent2D                     imageExtent
  //  1,                                            // uint32_t                       imageArrayLayers
  //  desired_usage,                                // VkImageUsageFlags              imageUsage
  //  VK_SHARING_MODE_EXCLUSIVE,                    // VkSharingMode                  imageSharingMode
  //  0,                                            // uint32_t                       queueFamilyIndexCount
  //  nullptr,                                      // const uint32_t                *pQueueFamilyIndices
  //  desired_transform,                            // VkSurfaceTransformFlagBitsKHR  preTransform
  //  VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,            // VkCompositeAlphaFlagBitsKHR    compositeAlpha
  //  desired_present_mode,                         // VkPresentModeKHR               presentMode
  //  VK_TRUE,                                      // VkBool32                       clipped
  //  old_swap_chain                                // VkSwapchainKHR                 oldSwapchain
  //};
  //
  //if (vkCreateSwapchainKHR(Vulkan.Device, &swap_chain_create_info, nullptr, &Vulkan.SwapChain) != VK_SUCCESS) {
  //  std::cout << "Could not create swap chain!" << std::endl;
  //  return false;
  //}
  //if (old_swap_chain != VK_NULL_HANDLE) {
  //  vkDestroySwapchainKHR(Vulkan.Device, old_swap_chain, nullptr);
  //}
  //
  //CanRender = true;

  return true;
}

uint32_t Vulkan::GetSwapChainNumImages(VkSurfaceCapabilitiesKHR& surface_capabilities)
{
  //Image ~= FrameBuffer
  //(ex) needs at least 2 for double buffer
  uint32_t image_count = surface_capabilities.minImageCount + 1; //asking for one more
  if ((surface_capabilities.maxImageCount > 0) && (image_count > surface_capabilities.maxImageCount))
    image_count = surface_capabilities.maxImageCount;
  return image_count;
}

VkSurfaceFormatKHR Vulkan::GetSwapChainFormat(std::vector<VkSurfaceFormatKHR>& surface_formats)
{
  // If the list contains only one entry with undefined format
  // it means that there are no preferred surface formats and any can be chosen
  if ((surface_formats.size() == 1) && (surface_formats[0].format == VK_FORMAT_UNDEFINED))
    return{ VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };

  // Check if list contains most widely used R8 G8 B8 A8 format
  // with nonlinear color space
  for (VkSurfaceFormatKHR &surface_format : surface_formats)
    if (surface_format.format == VK_FORMAT_R8G8B8A8_UNORM)
      return surface_format;

  // Return the first format from the list
  return surface_formats[0];
}

VkExtent2D Vulkan::GetSwapChainExtent(VkSurfaceCapabilitiesKHR& surface_capabilities)
{
  // if width == height == -1, that means window size will match to swapchain's size
  // we define the size by ourselves but it must fit within defined confines
  if (surface_capabilities.currentExtent.width == -1)
  {
    VkExtent2D swap_chain_extent = { static_cast<uint32_t>(winAPI.Width()), static_cast<uint32_t>(winAPI.Height()) };
    if (swap_chain_extent.width < surface_capabilities.minImageExtent.width)
      swap_chain_extent.width = surface_capabilities.minImageExtent.width;
    if (swap_chain_extent.height < surface_capabilities.minImageExtent.height)
      swap_chain_extent.height = surface_capabilities.minImageExtent.height;
    if (swap_chain_extent.width > surface_capabilities.maxImageExtent.width)
      swap_chain_extent.width = surface_capabilities.maxImageExtent.width;
    if (swap_chain_extent.height > surface_capabilities.maxImageExtent.height)
      swap_chain_extent.height = surface_capabilities.maxImageExtent.height;
    return swap_chain_extent;
  }

  // if not, use window's size
  return surface_capabilities.currentExtent;
}

VkImageUsageFlags Vulkan::GetSwapChainUsageFlags(VkSurfaceCapabilitiesKHR& surface_capabilities)
{
  // VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT must always be supported
  // We can define other usage flags but we always need to check if they are supported
  if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  OutputDebugString("VK_IMAGE_USAGE_TRANSFER_DST image usage is not supported by the swap chain!\n");
  OutputDebugString("Supported swap chain's image usages include:\n");
  if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) 
    OutputDebugString("VK_IMAGE_USAGE_TRANSFER_SRC\n");
  if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    OutputDebugString("VK_IMAGE_USAGE_TRANSFER_DST\n");
  if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT) 
    OutputDebugString("VK_IMAGE_USAGE_SAMPLED\n");
  if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT) 
    OutputDebugString("VK_IMAGE_USAGE_STORAGE\n");
  if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)         
    OutputDebugString("VK_IMAGE_USAGE_COLOR_ATTACHMENT\n");
  if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    OutputDebugString("VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT\n");
  if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
    OutputDebugString("VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT\n");
  if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
    OutputDebugString("VK_IMAGE_USAGE_INPUT_ATTACHMENT\n");

  return static_cast<VkImageUsageFlags>(-1);
}

VkSurfaceTransformFlagBitsKHR Vulkan::GetSwapChainTransform(VkSurfaceCapabilitiesKHR &surface_capabilities)
{
  // Sometimes images must be transformed before they are presented
  // portrait mode to landscape mode (ex)

  // If the specified transform is other than current transform, presentation engine will transform image
  // during presentation operation; this operation may hit performance on some platforms
  
  // Here we don't want any transformations to occur so if the identity transform is supported use it
  // otherwise just use the same transform as current transform
  if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  else
    return surface_capabilities.currentTransform;
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