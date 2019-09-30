#include <string>
#include <vector>

#define VK_USE_PLATFORM_WIN32_KHR
#include "resources\vulkan\vulkan.h"
#include "system.h"
#include "assert.h"
#include "fileio.h"

#include "vkfirsttriangle.h"

#if VK_CURRENT_MODE == VK_FIRST_TRIANGLE

bool VKFirstTriangle::CreateDevice()
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

  queue_create_infos.push_back({ //peek VkDeviceQueueCreateInfo for more details
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

bool VKFirstTriangle::CheckPhysicalDeviceProperties(VkPhysicalDevice physical_device,
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
      // Select first queue family that supports graphics
      if (graphics_queue_family_index == UINT32_MAX)
        graphics_queue_family_index = i;

      // If there is queue family that supports both graphics and present - prefer it
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

bool VKFirstTriangle::GetDeviceQueue()
{
  vkGetDeviceQueue(m_device, m_graphics_queue_family_index, 0, &m_graphics_queue); //need to call this per loading queue!
  vkGetDeviceQueue(m_device, m_present_queue_family_index, 0, &m_present_queue);
  return true;
}

bool VKFirstTriangle::OnWindowSizeChanged()
{
  Clear();

  if (!CreateSwapChain())
    return false;

  if (m_swap_chain != VK_NULL_HANDLE)
  {
    if (!CreateRenderPass())
      return false;
  
    if (!CreateFrameBuffers())
      return false;
    
    if (!CreatePipeline())
      return false;
    
    if (!CreateCommandBuffers())
      return false;
    
    if (!RecordCommandBuffers())
      return false;
  }

  return true;
}

void VKFirstTriangle::Clear()
{
  if (m_device != VK_NULL_HANDLE)
  {
    vkDeviceWaitIdle(m_device);

    if ((m_graphics_queue_command_buffers.size() > 0) && (m_graphics_queue_command_buffers[0] != VK_NULL_HANDLE))
    {
      vkFreeCommandBuffers(m_device, m_graphics_queue_command_pool, static_cast<uint32_t>(m_graphics_queue_command_buffers.size()), m_graphics_queue_command_buffers.data());
      m_graphics_queue_command_buffers.clear();
    }

    if (m_graphics_queue_command_pool != VK_NULL_HANDLE)
    {
      vkDestroyCommandPool(m_device, m_graphics_queue_command_pool, nullptr);
      m_graphics_queue_command_pool = VK_NULL_HANDLE;
    }

    if (m_pipeline != VK_NULL_HANDLE)
    {
      vkDestroyPipeline(m_device, m_pipeline, nullptr);
      m_pipeline = VK_NULL_HANDLE;
    }

    if (m_render_pass != VK_NULL_HANDLE)
    {
      vkDestroyRenderPass(m_device, m_render_pass, nullptr);
      m_render_pass = VK_NULL_HANDLE;
    }

    for (size_t i = 0; i < m_frame_buffers.size(); ++i)
      if (m_frame_buffers[i] != VK_NULL_HANDLE)
        vkDestroyFramebuffer(m_device, m_frame_buffers[i], nullptr);
    m_frame_buffers.clear();
  }
}

bool VKFirstTriangle::CreateSemaphores()
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

bool VKFirstTriangle::CreateSwapChain()
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
  VkSurfaceFormatKHR            desired_format = GetSwapChainFormat(surface_formats);
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
    desired_format.format,
    desired_format.colorSpace,
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
  
  m_swap_chain_format = desired_format.format;
  m_swap_chain_extent = desired_extent;

  uint32_t image_count = 0;
  if ((vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count, nullptr) != VK_SUCCESS) || (image_count == 0))
  {
    assert("Could not get swap chain images!", "Vulkan", Assert::Error);
    return false;
  }

  std::vector<VkImage> images(image_count);
  if (vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count, images.data()) != VK_SUCCESS)
  {
    assert("Could not get swap chain images!", "Vulkan", Assert::Error);
    return false;
  }

  m_swap_chain_images.resize(image_count);
  for (size_t i = 0; i < m_swap_chain_images.size(); ++i)
    m_swap_chain_images[i].handle = images[i];

  return CreateSwapChainImageViews();
}

uint32_t VKFirstTriangle::GetSwapChainNumImages(VkSurfaceCapabilitiesKHR& surface_capabilities)
{
  //Image ~= FrameBuffer
  //(ex) needs at least 2 for double buffer

  uint32_t image_count = surface_capabilities.minImageCount + 1; //asking for one more
  if ((surface_capabilities.maxImageCount > 0) && (image_count > surface_capabilities.maxImageCount))
    image_count = surface_capabilities.maxImageCount;

  OutputDebugString(("Available Swap Chain Images: " + std::to_string(image_count) + "\n").c_str());
  return image_count;
}

VkSurfaceFormatKHR VKFirstTriangle::GetSwapChainFormat(std::vector<VkSurfaceFormatKHR>& surface_formats)
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

VkExtent2D VKFirstTriangle::GetSwapChainExtent(VkSurfaceCapabilitiesKHR& surface_capabilities)
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

VkImageUsageFlags VKFirstTriangle::GetSwapChainUsageFlags(VkSurfaceCapabilitiesKHR& surface_capabilities)
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

VkSurfaceTransformFlagBitsKHR VKFirstTriangle::GetSwapChainTransform(VkSurfaceCapabilitiesKHR& surface_capabilities)
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

VkPresentModeKHR VKFirstTriangle::GetSwapChainPresentMode(std::vector<VkPresentModeKHR>& present_modes)
{
  // available present modes: Immediate, FIFO, FIFO(Relax), Mailbox
  // FIFO is always available
  // Mailbox is not, but more stable like triple-buffering (v-sync)

  // if supports mailbox, use it
  for (VkPresentModeKHR& present_mode : present_modes)
    if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
    {
      OutputDebugString("Present mode: Mailbox\n");
      return present_mode;
    }

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

bool VKFirstTriangle::CreateSwapChainImageViews()
{
  for (size_t i = 0; i < m_swap_chain_images.size(); ++i)
  {
    VkImageViewCreateInfo image_view_create_info = {
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,   // VkStructureType
      nullptr,                                    // pNext
      0,                                          // VkImageViewCreateFlags
      m_swap_chain_images[i].handle,              // VkImage
      VK_IMAGE_VIEW_TYPE_2D,                      // VkImageViewType
      m_swap_chain_format,                        // VkFormat
      {                                           // VkComponentMapping             components
        VK_COMPONENT_SWIZZLE_IDENTITY,              // VkComponentSwizzle             r
        VK_COMPONENT_SWIZZLE_IDENTITY,              // VkComponentSwizzle             g
        VK_COMPONENT_SWIZZLE_IDENTITY,              // VkComponentSwizzle             b
        VK_COMPONENT_SWIZZLE_IDENTITY               // VkComponentSwizzle             a, identity == using same value as above
      },
      {                                           // VkImageSubresourceRange        subresourceRange
        VK_IMAGE_ASPECT_COLOR_BIT,                  // VkImageAspectFlags             aspectMask
        0,                                          // uint32_t                       baseMipLevel
        1,                                          // uint32_t                       levelCount
        0,                                          // uint32_t                       baseArrayLayer
        1                                           // uint32_t                       layerCount
      }
    };

    if (vkCreateImageView(m_device, &image_view_create_info, nullptr, &m_swap_chain_images[i].view) != VK_SUCCESS)
    {
      assert("Could not create image view!", "Vulkan", Assert::Error);
      return false;
    }
  }

  return true;
}

bool VKFirstTriangle::CreateRenderPass()
{
  VkAttachmentDescription attachment_descriptions[1] = {
    {
      0,                                          // flags
      m_swap_chain_format,                        // format
      VK_SAMPLE_COUNT_1_BIT,                      // samples
      VK_ATTACHMENT_LOAD_OP_CLEAR,                // loadOp = operation on load, before renderpass, includes depth too
      VK_ATTACHMENT_STORE_OP_STORE,               // storeOp = operation after renderpass, includes depth too
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,            // stencilLoadOp
      VK_ATTACHMENT_STORE_OP_DONT_CARE,           // stencilStoreOp
      VK_IMAGE_LAYOUT_UNDEFINED,                  // initialLayout
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR             // finalLayout
    }
  };

  VkAttachmentReference color_attachment_references[1] = {
    {
      0,                                          // attachment index?
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL    // layout
    }
  };
  
  VkSubpassDescription subpass_descriptions[1] = {
    {
      0,                               // flags
      VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint, graphics or compute
      0,                               // inputAttachmentCount
      nullptr,                         // pInputAttachments
      1,                               // colorAttachmentCount
      color_attachment_references,     // pColorAttachments
      nullptr,                         // pResolveAttachments
      nullptr,                         // pDepthStencilAttachment
      0,                               // preserveAttachmentCount
      nullptr                          // pPreserveAttachments
    }
  };
  
  VkRenderPassCreateInfo render_pass_create_info = {
    VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,    // sType
    nullptr,                                      // pNext
    0,                                            // flags
    1,                                            // attachmentCount
    attachment_descriptions,                      // pAttachments
    1,                                            // subpassCount
    subpass_descriptions,                         // pSubpasses
    0,                                            // dependencyCount
    nullptr                                       // pDependencies, dependency between subpasses
  };
  
  if (vkCreateRenderPass(m_device, &render_pass_create_info, nullptr, &m_render_pass) != VK_SUCCESS)
  {
    assert("Could not create render pass!", "Vulkan", Assert::Error);
    return false;
  }

  return true;
}

bool VKFirstTriangle::CreateFrameBuffers()
{
  const std::vector<ImageInfo>& swap_chain_images = m_swap_chain_images;
  m_frame_buffers.resize(swap_chain_images.size());

  for(size_t i = 0; i < m_frame_buffers.size(); ++i)
  {
    VkFramebufferCreateInfo framebuffer_create_info = {
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, // VkStructureType
        nullptr,                                   // pNext
        0,                                         // VkFramebufferCreateFlags
        m_render_pass,                             // VkRenderPass
        1,                                         // attachmentCount, matched with render pass attachmentCount! (maybe subpass)
        &swap_chain_images[i].view,                // pAttachments
        static_cast<uint32_t>(winAPI.Width()),     // width
        static_cast<uint32_t>(winAPI.Height()),    // height
        1                                          // layers
    };

    if (vkCreateFramebuffer(m_device, &framebuffer_create_info, nullptr, &m_frame_buffers[i]) != VK_SUCCESS)
    {
      assert("Could not create frame buffer!", "Vulkan", Assert::Error);
      return false;
    }
  }

  return true;
}

bool VKFirstTriangle::CreatePipeline()
{
  VkShaderModule vertex_shader_module = CreateShaderModule("shaders/vert.spv");
  VkShaderModule fragment_shader_module = CreateShaderModule("shaders/frag.spv");

  if (!vertex_shader_module || !fragment_shader_module)
    return false;

  std::vector<VkPipelineShaderStageCreateInfo> shader_stage_create_infos = {
    // Vertex shader
    {
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // VkStructureType
      nullptr,                                             // pNext
      0,                                                   // VkPipelineShaderStageCreateFlags
      VK_SHADER_STAGE_VERTEX_BIT,                          // VkShaderStageFlagBits
      vertex_shader_module,                                // VkShaderModule
      "main",                                              // pName
      nullptr                                              // const VkSpecializationInfo
    },
    // Fragment shader
    {
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // VkStructureType
      nullptr,                                             // pNext
      0,                                                   // VkPipelineShaderStageCreateFlags
      VK_SHADER_STAGE_FRAGMENT_BIT,                        // VkShaderStageFlagBits
      fragment_shader_module,                              // VkShaderModule
      "main",                                              // pName
      nullptr                                              // const VkSpecializationInfo
    }
  };

  VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // VkStructureType
      nullptr,                                                   // *pNext
      0,                                                         // VkPipelineVertexInputStateCreateFlags
      0,                                                         // vertexBindingDescriptionCount
      nullptr,                                                   // const VkVertexInputBindingDescription*
      0,                                                         // vertexAttributeDescriptionCount
      nullptr                                                    // const VkVertexInputAttributeDescription*
  };

  VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {
  VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,  // VkStructureType
  nullptr,                                                      // pNext
  0,                                                            // VkPipelineInputAssemblyStateCreateFlags
  VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,                          // VkPrimitiveTopology
  VK_FALSE                                                      // primitiveRestartEnable
  };

  VkViewport viewport = {
  0.0f,                                // x
  0.0f,                                // y
  static_cast<float>(winAPI.Width()),  // width
  static_cast<float>(winAPI.Height()), // height
  0.0f,                                // minDepth, depth range is [0, 1)
  1.0f                                 // maxDepth
  };

  VkRect2D scissor = {
    {                                        // VkOffset2D
      0,                                       // x
      0                                        // y
    },                                       
    {                                        // VkExtent2D
      static_cast<uint32_t>(winAPI.Width()),   // width
      static_cast<uint32_t>(winAPI.Height())   // height
    }
  };

  VkPipelineViewportStateCreateInfo viewport_state_create_info = {
    VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, // VkStructureType
    nullptr,                                               // pNext
    0,                                                     // VkPipelineViewportStateCreateFlags
    1,                                                     // viewportCount
    &viewport,                                             // pViewports
    1,                                                     // scissorCount
    &scissor                                               // pScissors
  };

  VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {
  VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, // VkStructureType
  nullptr,                                                    // pNext
  0,                                                          // VkPipelineRasterizationStateCreateFlags
  VK_FALSE,                                                   // depthClampEnable
  VK_FALSE,                                                   // rasterizerDiscardEnable
  VK_POLYGON_MODE_FILL,                                       // VkPolygonMode
  VK_CULL_MODE_BACK_BIT,                                      // VkCullModeFlags
  VK_FRONT_FACE_COUNTER_CLOCKWISE,                            // VkFrontFace
  VK_FALSE,                                                   // depthBiasEnable
  0.0f,                                                       // depthBiasConstantFactor
  0.0f,                                                       // depthBiasClamp
  0.0f,                                                       // depthBiasSlopeFactor
  1.0f                                                        // lineWidth
  };

  VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {
  VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, // VkStructureType
  nullptr,                                                  // pNext
  0,                                                        // VkPipelineMultisampleStateCreateFlags
  VK_SAMPLE_COUNT_1_BIT,                                    // VkSampleCountFlagBits
  VK_FALSE,                                                 // sampleShadingEnable
  1.0f,                                                     // minSampleShading
  nullptr,                                                  // pSampleMask
  VK_FALSE,                                                 // alphaToCoverageEnable
  VK_FALSE                                                  // alphaToOneEnable
  };

  VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
  VK_FALSE,                                             // blendEnable
  VK_BLEND_FACTOR_ONE,                                  // srcColorBlendFactor
  VK_BLEND_FACTOR_ZERO,                                 // dstColorBlendFactor
  VK_BLEND_OP_ADD,                                      // colorBlendOp
  VK_BLEND_FACTOR_ONE,                                  // srcAlphaBlendFactor
  VK_BLEND_FACTOR_ZERO,                                 // dstAlphaBlendFactor
  VK_BLEND_OP_ADD,                                      // alphaBlendOp
  VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | // colorWriteMask
  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
  };

  VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {
    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, // VkStructureType
    nullptr,                                                  // pNext
    0,                                                        // VkPipelineColorBlendStateCreateFlags
    VK_FALSE,                                                 // logicOpEnable
    VK_LOGIC_OP_COPY,                                         // VkLogicOp
    1,                                                        // attachmentCount
    &color_blend_attachment_state,                            // pAttachments
    { 0.0f, 0.0f, 0.0f, 0.0f }                                // blendConstants[4]
  };

  VkPipelineLayout pipeline_layout = CreatePipelineLayout();
  if (!pipeline_layout)
    return false;

  VkGraphicsPipelineCreateInfo pipeline_create_info = {
    VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,         // VkStructureType
    nullptr,                                                 // pNext
    0,                                                       // VkPipelineCreateFlags
    static_cast<uint32_t>(shader_stage_create_infos.size()), // stageCount
    shader_stage_create_infos.data(),                        // pStages
    &vertex_input_state_create_info,                         // pVertexInputState
    &input_assembly_state_create_info,                       // pInputAssemblyState
    nullptr,                                                 // pTessellationState
    &viewport_state_create_info,                             // pViewportState
    &rasterization_state_create_info,                        // pRasterizationState
    &multisample_state_create_info,                          // pMultisampleState
    nullptr,                                                 // pDepthStencilState
    &color_blend_state_create_info,                          // pColorBlendState
    nullptr,                                                 // pDynamicState
    pipeline_layout,                                         // VkPipelineLayout
    m_render_pass,                                           // VkRenderPass
    0,                                                       // subpass
    VK_NULL_HANDLE,                                          // basePipelineHandle, well... pipeline can 'Inherit' pipelines...
    -1                                                       // basePipelineIndex
  };

  if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &m_pipeline) != VK_SUCCESS)
  {
    assert("Could not create graphics pipeline!", "Vulkan", Assert::Error);
    return false;
  }

  vkDestroyShaderModule(m_device, vertex_shader_module, nullptr);
  vkDestroyShaderModule(m_device, fragment_shader_module, nullptr);
  vkDestroyPipelineLayout(m_device, pipeline_layout, nullptr);

  return true;
}

VkShaderModule VKFirstTriangle::CreateShaderModule(const char* file_name)
{
  if (file_name == nullptr)
    return nullptr;

  std::vector<char>&& code = read_binary_file(file_name);

  if (code.empty())
    return nullptr;

  VkShaderModuleCreateInfo shader_module_create_info = {
    VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,    // VkStructureType
    nullptr,                                        // pNext
    0,                                              // VkShaderModuleCreateFlags
    code.size(),                                    // codeSize
    reinterpret_cast<const uint32_t*>(code.data())  // pCode
  };

  VkShaderModule shader_module;
  if (vkCreateShaderModule(m_device, &shader_module_create_info, nullptr, &shader_module) != VK_SUCCESS)
  {
    assert("Could not create shader module!", "Vulkan", Assert::Error);
    return nullptr;
  }

  return shader_module;
}

VkPipelineLayout VKFirstTriangle::CreatePipelineLayout()
{
  VkPipelineLayoutCreateInfo layout_create_info = {
  VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, // VkStructureType
  nullptr,                                       // pNext
  0,                                             // VkPipelineLayoutCreateFlags
  0,                                             // setLayoutCount
  nullptr,                                       // pSetLayouts
  0,                                             // pushConstantRangeCount
  nullptr                                        // pPushConstantRanges
  };

  VkPipelineLayout pipeline_layout;
  if (vkCreatePipelineLayout(m_device, &layout_create_info, nullptr, &pipeline_layout) != VK_SUCCESS)
  {
    assert("Could not create pipeline layout!", "Vulkan", Assert::Error);
    return nullptr;
  }

  return pipeline_layout;
}

bool VKFirstTriangle::CreateCommandBuffers()
{
  if (!CreateCommandPool(m_graphics_queue_family_index, &m_graphics_queue_command_pool))
    return false;

  uint32_t image_count = static_cast<uint32_t>(m_swap_chain_images.size());
  m_graphics_queue_command_buffers.resize(image_count, VK_NULL_HANDLE);

  if (!AllocateCommandBuffers(m_graphics_queue_command_pool, image_count, m_graphics_queue_command_buffers.data()))
    return false;
  return true;
}

bool VKFirstTriangle::CreateCommandPool(uint32_t queue_family_index, VkCommandPool* pool)
{
  VkCommandPoolCreateInfo cmd_pool_create_info = {
    VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, // VkStructureType
    nullptr,                                    // pNext
    0,                                          // VkCommandPoolCreateFlags flags
    queue_family_index                          // queueFamilyIndex
  };

  if (vkCreateCommandPool(m_device, &cmd_pool_create_info, nullptr, pool) != VK_SUCCESS)
  {
    assert("Could not create a command pool!", "Vulkan", Assert::Error);
    return false;
  }
  return true;
}

bool VKFirstTriangle::AllocateCommandBuffers(VkCommandPool pool, uint32_t count, VkCommandBuffer* command_buffers)
{
  VkCommandBufferAllocateInfo command_buffer_allocate_info = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, // VkStructureType
    nullptr,                                        // pNext
    pool,                                           // VkCommandPool
    VK_COMMAND_BUFFER_LEVEL_PRIMARY,                // VkCommandBufferLevel
    count                                           // bufferCount
  };

  if (vkAllocateCommandBuffers(m_device, &command_buffer_allocate_info, command_buffers) != VK_SUCCESS)
  {
    assert("Could not allocate command buffers!", "Vulkan", Assert::Error);
    return false;
  }
  return true;
}

bool VKFirstTriangle::RecordCommandBuffers()
{
  VkCommandBufferBeginInfo graphics_command_buffer_begin_info = {
  VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,  // VkStructureType
  nullptr,                                      // pNext
  VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, // VkCommandBufferUsageFlags
  nullptr                                       // pInheritanceInfo
  };

  VkImageSubresourceRange image_subresource_range = {
    VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
    0,                         // baseMipLevel
    1,                         // levelCount
    0,                         // baseArrayLayer
    1                          // layerCount
  };

  VkClearValue clear_value = {{ 1.0f, 0.8f, 0.4f, 0.0f }};
  
  for (size_t i = 0; i < m_graphics_queue_command_buffers.size(); ++i)
  {
    vkBeginCommandBuffer(m_graphics_queue_command_buffers[i], &graphics_command_buffer_begin_info);

    // move image between queues, if queue is different
    if (m_present_queue != m_graphics_queue)
    {
      VkImageMemoryBarrier barrier_from_present_to_graphics = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // VkStructureType
        nullptr,                                // pNext
        VK_ACCESS_MEMORY_READ_BIT,              // srcAccessMask
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,   // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,              // oldLayout
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,        // newLayout
        m_present_queue_family_index,           // srcQueueFamilyIndex
        m_graphics_queue_family_index,          // dstQueueFamilyIndex
        m_swap_chain_images[i].handle,          // image
        image_subresource_range                 // VkImageSubresourceRange
      };
      vkCmdPipelineBarrier(m_graphics_queue_command_buffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier_from_present_to_graphics);
    }

    VkRenderPassBeginInfo render_pass_begin_info = {
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // VkStructureType
      nullptr,                                   // pNext
      m_render_pass,                             // VkRenderPass
      m_frame_buffers[i],                        // VkFramebuffer
      {                                          // renderArea
        {                                        // VkOffset2D
          0,                                       // x
          0                                        // y
        },                                       
        {                                        // VkExtent2D
          static_cast<uint32_t>(winAPI.Width()),   // width
          static_cast<uint32_t>(winAPI.Height())   // height
        }                                        
      },                                         
      1,                                         // clearValueCount
      &clear_value                               // pClearValues
    };

    vkCmdBeginRenderPass(m_graphics_queue_command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(m_graphics_queue_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    vkCmdDraw(m_graphics_queue_command_buffers[i], 3, 1, 0, 0);

    vkCmdEndRenderPass(m_graphics_queue_command_buffers[i]);

    if (m_present_queue != m_graphics_queue)
    {
      VkImageMemoryBarrier barrier_from_graphics_to_present = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // VkStructureType
        nullptr,                                // pNext
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,   // srcAccessMask
        VK_ACCESS_MEMORY_READ_BIT,              // dstAccessMask
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,        // oldLayout
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,        // newLayout
        m_graphics_queue_family_index,          // srcQueueFamilyIndex
        m_present_queue_family_index,           // dstQueueFamilyIndex
        m_swap_chain_images[i].handle,          // VkImage
        image_subresource_range                 // VkImageSubresourceRange
      };
      vkCmdPipelineBarrier(m_graphics_queue_command_buffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier_from_graphics_to_present);
    }

    if (vkEndCommandBuffer(m_graphics_queue_command_buffers[i]) != VK_SUCCESS)
    {
      assert("Could not record command buffer!", "Vulkan", Assert::Error);
      return false;
    }
  }
  return true;
}

bool VKFirstTriangle::Initialize()
{
#define CHECK(x) if (!x()) return false;

  CHECK(GetDeviceQueue)
  CHECK(CreateSemaphores)
  CHECK(OnWindowSizeChanged)

#undef CHECK

  return true;
}

void VKFirstTriangle::Update()
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
  }

  VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submit_info = {
    VK_STRUCTURE_TYPE_SUBMIT_INFO,                 // VkStructureType
    nullptr,                                       // pNext
    1,                                             // waitSemaphoreCount
    &m_image_available_semaphore,                  // pWaitSemaphores
    &wait_dst_stage_mask,                          // pWaitDstStageMask
    1,                                             // commandBufferCount
    &m_graphics_queue_command_buffers[image_index],// pCommandBuffers
    1,                                             // signalSemaphoreCount
    &m_rendering_finished_semaphore                // pSignalSemaphores
  };

  if (vkQueueSubmit(m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS)
  {
    assert("Problem occurred during graphics queue submit!", "Vulkan", Assert::Error);
    engine.Quit();
    return;
  }

  VkPresentInfoKHR present_info = {
    VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, // sType
    nullptr,                            // pNext
    1,                                  // waitSemaphoreCount
    &m_rendering_finished_semaphore,    // pWaitSemaphores
    1,                                  // swapchainCount
    &m_swap_chain,                      // pSwapchains
    &image_index,                       // pImageIndices
    nullptr                             // pResults
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
    assert("Problem occurred during present!", "Vulkan", Assert::Error);
    engine.Quit();
    return;
  }

  return;
}

void VKFirstTriangle::Terminate()
{
  Clear();

  //warning! all releasing should be in order!
  if (m_device != VK_NULL_HANDLE)
  {
    vkDeviceWaitIdle(m_device);

    if (m_image_available_semaphore != VK_NULL_HANDLE)
    {
      vkDestroySemaphore(m_device, m_image_available_semaphore, nullptr);
      m_image_available_semaphore = VK_NULL_HANDLE;
    }

    if (m_rendering_finished_semaphore != VK_NULL_HANDLE)
    {
      vkDestroySemaphore(m_device, m_rendering_finished_semaphore, nullptr);
      m_rendering_finished_semaphore = VK_NULL_HANDLE;
    }

    if (m_swap_chain != VK_NULL_HANDLE)
    {
      vkDestroySwapchainKHR(m_device, m_swap_chain, nullptr);
      m_swap_chain = VK_NULL_HANDLE;
    }

    for (size_t i = 0; i < m_swap_chain_images.size(); ++i)
      if (m_swap_chain_images[i].view != VK_NULL_HANDLE)
        vkDestroyImageView(m_device, m_swap_chain_images[i].view, nullptr);
    m_swap_chain_images.clear();
  }
}

#endif