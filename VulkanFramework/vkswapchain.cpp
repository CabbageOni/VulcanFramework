
#include <string>
#include <vector>

#define VK_USE_PLATFORM_WIN32_KHR
#include "resources\vulkan\vulkan.h"
#include "system.h"
#include "assert.h"

#include "vkswapchain.h"

#if VK_CURRENT_MODE == VK_SWAPCHAIN

bool VKSwapChain::CreateInstance()
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
  &extensions[0] //enabledExtensionNames
  };

  if (vkCreateInstance(&instance_create_info, nullptr, &m_instance) != VK_SUCCESS)
  {
    assert("Could not create Vulkan instance!", "Vulkan", Assert::Error);
    return false;
  }

  return true;
}

bool VKSwapChain::CheckExtensionAvailability(const char* extension_name, const std::vector<VkExtensionProperties>& available_extensions)
{
  for (size_t i = 0; i < available_extensions.size(); ++i)
    if (strcmp(available_extensions[i].extensionName, extension_name) == 0)
      return true;
return false;
}

bool VKSwapChain::CreateDevice()
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

bool VKSwapChain::CheckPhysicalDeviceProperties(VkPhysicalDevice physical_device,
uint32_t& selected_graphics_queue_family_index,
uint32_t& selected_present_queue_family_index)
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
  //uint32_t minor_version = VK_VERSION_MINOR(device_properties.apiVersion);
  //uint32_t patch_version = VK_VERSION_PATCH(device_properties.apiVersion);

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
        graphics_queue_family_index = i;

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

bool VKSwapChain::GetDeviceQueue()
{
  vkGetDeviceQueue(m_device, m_graphics_queue_family_index, 0, &m_graphics_queue); //need to call this per loading queue!
  vkGetDeviceQueue(m_device, m_present_queue_family_index, 0, &m_present_queue);
  return true;
}

bool VKSwapChain::OnWindowSizeChanged()
{
  Clear();

  if (!CreateSwapChain())
    return false;

  if (!CreateCommandBuffers())
    return false;

  return true;
}

void VKSwapChain::Clear()
{
  if (m_device != VK_NULL_HANDLE)
    vkDeviceWaitIdle(m_device);
 
  if ((m_present_queue_cmd_buffers.size() > 0) && (m_present_queue_cmd_buffers[0] != VK_NULL_HANDLE))
  {
    vkFreeCommandBuffers(m_device, m_present_queue_cmd_pool, static_cast<uint32_t>(m_present_queue_cmd_buffers.size()), m_present_queue_cmd_buffers.data());
    m_present_queue_cmd_buffers.clear();
  }

  if (m_present_queue_cmd_pool != VK_NULL_HANDLE)
  {
    vkDestroyCommandPool(m_device, m_present_queue_cmd_pool, nullptr);
    m_present_queue_cmd_pool = VK_NULL_HANDLE;
  }
}

bool VKSwapChain::CreatePresentationSurface()
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

bool VKSwapChain::CreateSemaphores()
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

bool VKSwapChain::CreateSwapChain()
{
  OutputDebugString("Recreating Swapchain...\n");

  if (m_device != VK_NULL_HANDLE)
    vkDeviceWaitIdle(m_device);

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
  if ((vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_presentation_surface, &present_modes_count, nullptr) != VK_SUCCESS) ||
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

  VkImageUsageFlags             desired_usage = GetSwapChainUsageFlags(surface_capabilities);
  VkPresentModeKHR              desired_present_mode = GetSwapChainPresentMode(present_modes);
  VkExtent2D                    desired_extent = GetSwapChainExtent(surface_capabilities);
  uint32_t                      desired_number_of_images = GetSwapChainNumImages(surface_capabilities);
  m_swap_chain_format = GetSwapChainFormat(surface_formats);
  VkSurfaceTransformFlagBitsKHR desired_transform = GetSwapChainTransform(surface_capabilities);
  VkSwapchainKHR                old_swap_chain = m_swap_chain;

  if (static_cast<int>(desired_usage) == -1)
    return false;

  if (static_cast<int>(desired_present_mode) == -1)
    return false;

  // (0,0) window size can happen (like when minimized), just don't render
  if ((desired_extent.width == 0) || (desired_extent.height == 0))
    return true;

  VkSwapchainCreateInfoKHR swap_chain_create_info = { //peek VKFirstTriangleCreateInfoKHR for detail
    VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    nullptr, 0,
    m_presentation_surface,
    desired_number_of_images,
    m_swap_chain_format.format,
    m_swap_chain_format.colorSpace,
    desired_extent,
    1,
    desired_usage,
    VK_SHARING_MODE_EXCLUSIVE, //exclusive: other queues can refer to images, but cannot do at once. (barrier feature in thread-like)
    0, nullptr, //needed if Sharing mode is concurrent, need to sync many queues from different queue families to avoid thread-like problem
    desired_transform,
    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    desired_present_mode,
    VK_TRUE,
    old_swap_chain
  };

  if (vkCreateSwapchainKHR(m_device, &swap_chain_create_info, nullptr, &m_swap_chain) != VK_SUCCESS)
  {
    assert("Could not create swap chain!", "Vulkan", Assert::Error);
    return false;
  }

  //release old swap chain
  if (old_swap_chain != VK_NULL_HANDLE)
    vkDestroySwapchainKHR(m_device, old_swap_chain, nullptr);

  return true;
}

bool VKSwapChain::CreateCommandBuffers()
{
  VkCommandPoolCreateInfo cmd_pool_create_info = { // peek VkCommandPoolCreateInfo for more detail
    VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    nullptr, 0,
    m_present_queue_family_index
  };

  if (vkCreateCommandPool(m_device, &cmd_pool_create_info, nullptr, &m_present_queue_cmd_pool) != VK_SUCCESS)
  {
    assert("Could not create a command pool!", "Vulkan", Assert::Error);
    return false;
  }

  uint32_t image_count = 0;
  if ((vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count, nullptr) != VK_SUCCESS) || (image_count == 0))
  {
    assert("Could not get the number of swap chain images!", "Vulkan", Assert::Error);
    return false;
  }

  m_present_queue_cmd_buffers.resize(image_count);

  VkCommandBufferAllocateInfo cmd_buffer_allocate_info = { //peek VkCommandBufferAllocateInfo for more detail
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, 
    nullptr,                                       
    m_present_queue_cmd_pool,
    VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    image_count //buffercount
  };

  if (vkAllocateCommandBuffers(m_device, &cmd_buffer_allocate_info, m_present_queue_cmd_buffers.data()) != VK_SUCCESS)
  {
    assert("Could not allocate command buffers!", "Vulkan", Assert::Error);
    return false;
  }

  if (!RecordCommandBuffers())
  {
    assert("Could not record command buffers!", "Vulkan", Assert::Error);
    return false;
  }
  return true;
}

bool VKSwapChain::RecordCommandBuffers()
{
  uint32_t image_count = static_cast<uint32_t>(m_present_queue_cmd_buffers.size());

  std::vector<VkImage> swap_chain_images(image_count);
  if (vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count, swap_chain_images.data()) != VK_SUCCESS)
  {
    assert("Could not get swap chain images!", "Vulkan", Assert::Error);
    return false;
  }

  VkCommandBufferBeginInfo cmd_buffer_begin_info = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    nullptr,                                    
    VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
    nullptr                                    
  };

  VkClearColorValue clear_color = {
    { 1.0f, 0.8f, 0.4f, 0.0f }
  };

  VkImageSubresourceRange image_subresource_range = {
    VK_IMAGE_ASPECT_COLOR_BIT, 
    0,                         
    1,                         
    0,                         
    1                          
  };

  for (uint32_t i = 0; i < image_count; ++i)
  {
    VkImageMemoryBarrier barrier_from_present_to_clear = {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,     //  sType
      nullptr,                                    // *pNext
      VK_ACCESS_MEMORY_READ_BIT,                  //  srcAccessMask
      VK_ACCESS_TRANSFER_WRITE_BIT,               //  dstAccessMask
      VK_IMAGE_LAYOUT_UNDEFINED,                  //  oldLayout
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,       //  newLayout
      VK_QUEUE_FAMILY_IGNORED,                    //  srcQueueFamilyIndex
      VK_QUEUE_FAMILY_IGNORED,                    //  dstQueueFamilyIndex
      swap_chain_images[i],                       //  image
      image_subresource_range                     //  subresourceRange
    };

    VkImageMemoryBarrier barrier_from_clear_to_present = {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,     // VkStructureType                        sType
      nullptr,                                    // const void                            *pNext
      VK_ACCESS_TRANSFER_WRITE_BIT,               // VkAccessFlags                          srcAccessMask
      VK_ACCESS_MEMORY_READ_BIT,                  // VkAccessFlags                          dstAccessMask
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,       // VkImageLayout                          oldLayout
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,            // VkImageLayout                          newLayout
      VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                               srcQueueFamilyIndex
      VK_QUEUE_FAMILY_IGNORED,                    // uint32_t                               dstQueueFamilyIndex
      swap_chain_images[i],                       // VkImage                                image
      image_subresource_range                     // VkImageSubresourceRange                subresourceRange
    };
  
    vkBeginCommandBuffer(m_present_queue_cmd_buffers[i], &cmd_buffer_begin_info);
    vkCmdPipelineBarrier(m_present_queue_cmd_buffers[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier_from_present_to_clear);

    vkCmdClearColorImage(m_present_queue_cmd_buffers[i], swap_chain_images[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &image_subresource_range);
    vkCmdPipelineBarrier(m_present_queue_cmd_buffers[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier_from_clear_to_present);

    if (vkEndCommandBuffer(m_present_queue_cmd_buffers[i]) != VK_SUCCESS)
    {
      assert("Could not record command buffers!", "Vulkan", Assert::Error);
      return false;
    }
  }

  return true;
}

uint32_t VKSwapChain::GetSwapChainNumImages(VkSurfaceCapabilitiesKHR& surface_capabilities)
{
  //Image ~= FrameBuffer
  //(ex) needs at least 2 for double buffer

  uint32_t image_count = surface_capabilities.minImageCount + 1; //asking for one more
  if ((surface_capabilities.maxImageCount > 0) && (image_count > surface_capabilities.maxImageCount))
    image_count = surface_capabilities.maxImageCount;

  OutputDebugString(("Available Swap Chain Images: " + std::to_string(image_count) + "\n").c_str());
  return image_count;
}

VkSurfaceFormatKHR VKSwapChain::GetSwapChainFormat(std::vector<VkSurfaceFormatKHR>& surface_formats)
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

VkExtent2D VKSwapChain::GetSwapChainExtent(VkSurfaceCapabilitiesKHR& surface_capabilities)
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

VkImageUsageFlags VKSwapChain::GetSwapChainUsageFlags(VkSurfaceCapabilitiesKHR& surface_capabilities)
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

VkSurfaceTransformFlagBitsKHR VKSwapChain::GetSwapChainTransform(VkSurfaceCapabilitiesKHR& surface_capabilities)
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

VkPresentModeKHR VKSwapChain::GetSwapChainPresentMode(std::vector<VkPresentModeKHR>& present_modes)
{
  // available present modes: Immediate, FIFO(Relax), Mailbox
  // FIFO is always available
  // Mailbox is not, but more stable like triple-buffering (v-sync)

  //if supports mailbox, use it
  //for (VkPresentModeKHR &present_mode : present_modes)
  //  if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
  //  {
  //    OutputDebugString("Present mode: Mailbox\n");
  //    return present_mode;
  //  }

  //otherwise use fifo
  for (VkPresentModeKHR &present_mode : present_modes)
    if (present_mode == VK_PRESENT_MODE_FIFO_KHR)
    {
      OutputDebugString("Present mode: FIFO\n");
      return present_mode;
    }

  assert("FIFO present mode is not supported by the swap chain!", "Vulkan", Assert::Error);
  return static_cast<VkPresentModeKHR>(-1);
}

bool VKSwapChain::Initialize()
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

CHECK(CreateSwapChain)
CHECK(CreateCommandBuffers)

#undef CHECK

return true;
}

void VKSwapChain::Update()
{
  uint32_t image_index;
  
  // acquiring next image to draw
  // may wait for presentation engine to finish its job
  VkResult result = vkAcquireNextImageKHR(m_device,
                                          m_swap_chain,
                                          UINT64_MAX, //timeout, max time wait for next image
                                          m_image_available_semaphore,
                                          VK_NULL_HANDLE,
                                          &image_index);
  
  switch (result)
  {
  case VK_SUCCESS:
  case VK_SUBOPTIMAL_KHR:
    break;
  case VK_ERROR_OUT_OF_DATE_KHR:
    if (!OnWindowSizeChanged())
      engine.Quit();
    return;
  default:
    assert("Problem occurred during swap chain image acquisition!", "Vulkan", Assert::Error);
    engine.Quit();
    return;
    break;
  }

  VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
  VkSubmitInfo submit_info = { //peek VkSubmitInfo for detail
    VK_STRUCTURE_TYPE_SUBMIT_INFO,
    nullptr,
    1, // waitSemaphoreCount
    &m_image_available_semaphore, //wait
    &wait_dst_stage_mask,
    1, // commandBufferCount
    &m_present_queue_cmd_buffers[image_index],
    1, // signalSemaphoreCount
    &m_rendering_finished_semaphore //signal
  };

  if (vkQueueSubmit(m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS)
  {
    assert("Problem occurred during swap chain image submission!", "Vulkan", Assert::Error);
    engine.Quit();
    return;
  }

  //presenting image
  VkPresentInfoKHR present_info = { //peek VkPresentInfoKHR for detail
    VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    nullptr,
    1, // waitSemaphoreCount
    &m_rendering_finished_semaphore,
    1, // swapchainCount
    &m_swap_chain,
    &image_index, 
    nullptr
  };
  result = vkQueuePresentKHR(m_present_queue, &present_info);

  switch (result)
  {
  case VK_SUCCESS:
    break;
  case VK_ERROR_OUT_OF_DATE_KHR:
  case VK_SUBOPTIMAL_KHR:
    if (!OnWindowSizeChanged())
      engine.Quit();
    return;
  default:
    assert("Problem occurred during image presentation!", "Vulkan", Assert::Error);
    engine.Quit();
    return;
  }
}

void VKSwapChain::Terminate()
{
  Clear();

  //warning! all releasing should be in order!
  if (m_device != VK_NULL_HANDLE)
  {
    vkDeviceWaitIdle(m_device);

    if (m_image_available_semaphore != VK_NULL_HANDLE)
      vkDestroySemaphore(m_device, m_image_available_semaphore, nullptr);

    if (m_rendering_finished_semaphore != VK_NULL_HANDLE)
      vkDestroySemaphore(m_device, m_rendering_finished_semaphore, nullptr);

    if (m_swap_chain != VK_NULL_HANDLE)
      vkDestroySwapchainKHR(m_device, m_swap_chain, nullptr);

    vkDestroyDevice(m_device, nullptr);
  }

  if (m_presentation_surface != VK_NULL_HANDLE)
    vkDestroySurfaceKHR(m_instance, m_presentation_surface, nullptr);

  if (m_instance != VK_NULL_HANDLE)
    vkDestroyInstance(m_instance, nullptr);

  if (m_vulkan_library)
    FreeLibrary(m_vulkan_library);
}

#endif