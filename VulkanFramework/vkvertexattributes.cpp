#define VK_USE_PLATFORM_WIN32_KHR
#include "resources\vulkan\vulkan.h"
#include "system.h"
#include "assert.h"
#include "fileio.h"

#include "vkvertexattributes.h"

#if VK_CURRENT_MODE == VK_VERTEX_ATTRIBUTES

bool VKVertexAttributes::Initialize()
{
#define CHECK(x) if (!x()) return false;

  CHECK(GetDeviceQueue)
  CHECK(CreateVertexBuffer)
  CHECK(CreateStagingBuffer)
  CHECK(CreateCommandBuffers)
  CHECK(CopyVertexData)
  CHECK(CreateSemaphores)
  CHECK(CreateFences)
  CHECK(OnWindowSizeChanged)

#undef CHECK

  return true;
}

bool VKVertexAttributes::CreateDevice()
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

bool VKVertexAttributes::CheckPhysicalDeviceProperties(VkPhysicalDevice physical_device,
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

bool VKVertexAttributes::GetDeviceQueue()
{
  vkGetDeviceQueue(m_device, m_graphics_queue_family_index, 0, &m_graphics_queue);
  vkGetDeviceQueue(m_device, m_present_queue_family_index, 0, &m_present_queue);
  return true;
}

bool VKVertexAttributes::CreateVertexBuffer()
{
  m_vertex_buffer_info.size = sizeof(m_vertex_data);
  m_vertex_buffer_info.count = static_cast<uint32_t>(m_vertex_buffer_info.size / sizeof(VertexData));

  if (!CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertex_buffer, m_vertex_buffer_info))
    return false;
  
  return true;
}

bool VKVertexAttributes::CreateBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlagBits memoryProperty, VkBuffer& buffer, BufferInfo& buffer_info)
{
  VkBufferCreateInfo buffer_create_info = {
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, // VkStructureType
      nullptr,                              // pNext
      0,                                    // VkBufferCreateFlags
      buffer_info.size,                     // VkDeviceSize
      usage,                                // VkBufferUsageFlags
      VK_SHARING_MODE_EXCLUSIVE,            // VkSharingMode
      0,                                    // queueFamilyIndexCount
      nullptr                               // pQueueFamilyIndices
  };

  if (vkCreateBuffer(m_device, &buffer_create_info, nullptr, &buffer) != VK_SUCCESS)
  {
    assert("Could not create buffer!", "Vulkan", Assert::Error);
    return false;
  }

  if (!AllocateBufferMemory(buffer, memoryProperty, &buffer_info.memory))
  {
    assert("Could not allocate memory for a buffer!", "Vulkan", Assert::Error);
    return false;
  }

  if (vkBindBufferMemory(m_device, buffer, buffer_info.memory, 0) != VK_SUCCESS)
  {
    assert("Could not bind memory to a buffer!", "Vulkan", Assert::Error);
    return false;
  }

  return true;
}

bool VKVertexAttributes::AllocateBufferMemory(VkBuffer buffer, VkMemoryPropertyFlagBits property, VkDeviceMemory* memory)
{
  VkMemoryRequirements buffer_memory_requirements;
  vkGetBufferMemoryRequirements(m_device, buffer, &buffer_memory_requirements);

  VkPhysicalDeviceMemoryProperties memory_properties;
  vkGetPhysicalDeviceMemoryProperties(m_physical_device, &memory_properties);

  for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
  {
    if ((buffer_memory_requirements.memoryTypeBits & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & property))
    {
      VkMemoryAllocateInfo memory_allocate_info = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, // VkStructureType
        nullptr,                                // pNext
        buffer_memory_requirements.size,        // allocationSize
        i                                       // memoryTypeIndex
      };

      if (vkAllocateMemory(m_device, &memory_allocate_info, nullptr, memory) == VK_SUCCESS)
        return true;
    }
  }
  return false;
}

bool VKVertexAttributes::CreateStagingBuffer()
{
  m_staging_buffer_info.size = 4000;
  if (!CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_staging_buffer, m_staging_buffer_info))
    return false;

  return true;
}

bool VKVertexAttributes::CreateCommandBuffers()
{
  if (!CreateCommandPool(m_graphics_queue_family_index, &m_graphics_command_pool))
    return false;

  for (VirtualFrame& virtual_frame : m_virtual_frames)
    if (!AllocateCommandBuffers(m_graphics_command_pool, 1, &virtual_frame.command_buffer))
      return false;

  return true;
}

bool VKVertexAttributes::CreateCommandPool(uint32_t queue_family_index, VkCommandPool* pool)
{
  // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT = enable individual command buffer rerecord. If without, command buffers can only be rerecorded by resetting whole command pool
  // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT = the command buffer will only last for a short period. 

  VkCommandPoolCreateInfo cmd_pool_create_info = {
    VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,                                             // VkStructureType
    nullptr,                                                                                // pNext
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, // VkCommandPoolCreateFlags
    queue_family_index                                                                      // queueFamilyIndex
  };

  if (vkCreateCommandPool(m_device, &cmd_pool_create_info, nullptr, pool) != VK_SUCCESS)
  {
    assert("Could not create command pool!", "Vulkan", Assert::Error);
    return false;
  }
  return true;
}

bool VKVertexAttributes::AllocateCommandBuffers(VkCommandPool pool, uint32_t count, VkCommandBuffer* command_buffers)
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
    assert("Could not allocate command buffer!", "Vulkan", Assert::Error);
    return false;
  }
  return true;
}

bool VKVertexAttributes::CopyVertexData()
{
  void* staging_buffer_memory_pointer;
  if (vkMapMemory(m_device, m_staging_buffer_info.memory, 0, m_vertex_buffer_info.size, 0, &staging_buffer_memory_pointer) != VK_SUCCESS)
  {
    assert("Could not map memory!", "Vulkan", Assert::Error);
    return false;
  }

  memcpy(staging_buffer_memory_pointer, m_vertex_data, m_vertex_buffer_info.size);

  VkMappedMemoryRange flush_range = {
    VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, // VkStructureType
    nullptr,                               // pNext
    m_staging_buffer_info.memory,          // memory
    0,                                     // offset
    m_vertex_buffer_info.size              // size
  };
  vkFlushMappedMemoryRanges(m_device, 1, &flush_range);

  vkUnmapMemory(m_device, m_staging_buffer_info.memory);

  // Prepare command buffer to copy data from staging buffer to a vertex buffer
  VkCommandBufferBeginInfo command_buffer_begin_info = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, // VkStructureType
    nullptr,                                     // pNext
    VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // VkCommandBufferUsageFlags
    nullptr                                      // pInheritanceInfo
  };

  VkCommandBuffer command_buffer = m_virtual_frames[0].command_buffer;

  vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

  VkBufferCopy buffer_copy_info = {
    0,                        // srcOffset
    0,                        // dstOffset
    m_vertex_buffer_info.size // size
  };
  vkCmdCopyBuffer(command_buffer, m_staging_buffer, m_vertex_buffer, 1, &buffer_copy_info);

  VkBufferMemoryBarrier buffer_memory_barrier = {
    VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType
    nullptr,                                 // pNext
    VK_ACCESS_MEMORY_WRITE_BIT,              // srcAccessMask
    VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,     // dstAccessMask
    VK_QUEUE_FAMILY_IGNORED,                 // srcQueueFamilyIndex
    VK_QUEUE_FAMILY_IGNORED,                 // dstQueueFamilyIndex
    m_vertex_buffer,                         // buffer
    0,                                       // offset
    VK_WHOLE_SIZE                            // size
  };
  vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

  vkEndCommandBuffer(command_buffer);

  // Submit command buffer and copy data from staging buffer to a vertex buffer
  VkSubmitInfo submit_info = {
    VK_STRUCTURE_TYPE_SUBMIT_INFO, // VkStructureType
    nullptr,                       // pNext
    0,                             // waitSemaphoreCount
    nullptr,                       // pWaitSemaphores
    nullptr,                       // pWaitDstStageMask;
    1,                             // commandBufferCount
    &command_buffer,               // pCommandBuffers
    0,                             // signalSemaphoreCount
    nullptr                        // pSignalSemaphores
  };

  if (vkQueueSubmit(m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS)
  {
    assert("Failed to submit buffer staging command buffer!", "Vulkan", Assert::Error);
    return false;
  }

  vkDeviceWaitIdle(m_device); // using a fence is a better option

  return true;
}

bool VKVertexAttributes::CreateSemaphores()
{
  VkSemaphoreCreateInfo semaphore_create_info = {
    VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, // VkStructureType
    nullptr,                                 // pNext
    0                                        // VkSemaphoreCreateFlags
  };

  for (VirtualFrame& virtual_frame : m_virtual_frames) 
    if ((vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &virtual_frame.image_available_semaphore) != VK_SUCCESS) ||
      (vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &virtual_frame.finished_rendering_semaphore) != VK_SUCCESS))
    {
      assert("Could not create semaphores!", "Vulkan", Assert::Error);
      return false;
    }
  
  return true;
}

bool VKVertexAttributes::CreateFences()
{
  VkFenceCreateInfo fence_create_info = {
    VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, // VkStructureType
    nullptr,                             // pNext
    VK_FENCE_CREATE_SIGNALED_BIT         // VkFenceCreateFlags
  };

  for (VirtualFrame& virtual_frame : m_virtual_frames)
    if (vkCreateFence(m_device, &fence_create_info, nullptr, &virtual_frame.fence) != VK_SUCCESS)
    {
      assert("Could not create a fence!", "Vulkan", Assert::Error);
      return false;
    }

  return true;
}

bool VKVertexAttributes::OnWindowSizeChanged()
{
  Clear();

  if (!CreateSwapChain())
    return false;

  if (m_swap_chain != VK_NULL_HANDLE)
  {
    if (!CreateRenderPass())
      return false;

    if (!CreatePipeline())
      return false;
  }

  return true;
}

void VKVertexAttributes::Clear()
{
  if (m_device != VK_NULL_HANDLE)
  {
    vkDeviceWaitIdle(m_device);

    // release swap chain images and its related
    m_swap_chain_info.images.clear();

    for (size_t i = 0; i < m_swap_chain_info.image_views.size(); ++i)
      if (m_swap_chain_info.image_views[i] != VK_NULL_HANDLE)
        vkDestroyImageView(m_device, m_swap_chain_info.image_views[i], nullptr);
    m_swap_chain_info.image_views.clear();

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
  }
}

bool VKVertexAttributes::CreateSwapChain()
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
 
  m_swap_chain_info.format = desired_format.format;
  m_swap_chain_info.extent = desired_extent;

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

  m_swap_chain_info.images.resize(image_count);
  for (size_t i = 0; i < image_count; ++i)
    m_swap_chain_info.images[i] = images[i];

  return CreateSwapChainImageViews();
}

uint32_t VKVertexAttributes::GetSwapChainNumImages(VkSurfaceCapabilitiesKHR& surface_capabilities)
{
  //Image ~= FrameBuffer
  //(ex) needs at least 2 for double buffer

  uint32_t image_count = surface_capabilities.minImageCount + 1; //asking for one more
  if ((surface_capabilities.maxImageCount > 0) && (image_count > surface_capabilities.maxImageCount))
    image_count = surface_capabilities.maxImageCount;

  OutputDebugString(("Available Swap Chain Images: " + std::to_string(image_count) + "\n").c_str());
  return image_count;
}

VkSurfaceFormatKHR VKVertexAttributes::GetSwapChainFormat(std::vector<VkSurfaceFormatKHR>& surface_formats)
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

VkExtent2D VKVertexAttributes::GetSwapChainExtent(VkSurfaceCapabilitiesKHR& surface_capabilities)
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

VkImageUsageFlags VKVertexAttributes::GetSwapChainUsageFlags(VkSurfaceCapabilitiesKHR& surface_capabilities)
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

VkSurfaceTransformFlagBitsKHR VKVertexAttributes::GetSwapChainTransform(VkSurfaceCapabilitiesKHR& surface_capabilities)
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

VkPresentModeKHR VKVertexAttributes::GetSwapChainPresentMode(std::vector<VkPresentModeKHR>& present_modes)
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

bool VKVertexAttributes::CreateSwapChainImageViews()
{
  m_swap_chain_info.image_views.resize(m_swap_chain_info.images.size());
  for (size_t i = 0; i < m_swap_chain_info.images.size(); ++i)
  {
    VkImageViewCreateInfo image_view_create_info = {
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,   // VkStructureType
      nullptr,                                    // pNext
      0,                                          // VkImageViewCreateFlags
      m_swap_chain_info.images[i],                // VkImage
      VK_IMAGE_VIEW_TYPE_2D,                      // VkImageViewType
      m_swap_chain_info.format,                   // VkFormat
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

    if (vkCreateImageView(m_device, &image_view_create_info, nullptr, &m_swap_chain_info.image_views[i]) != VK_SUCCESS)
    {
      assert("Could not create image view!", "Vulkan", Assert::Error);
      return false;
    }
  }

  return true;
}

bool VKVertexAttributes::CreateRenderPass()
{
  VkAttachmentDescription attachment_descriptions[] = {
    {
      0,                                // VkAttachmentDescriptionFlags
      m_swap_chain_info.format,         // VkFormat
      VK_SAMPLE_COUNT_1_BIT,            // VkSampleCountFlagBits
      VK_ATTACHMENT_LOAD_OP_CLEAR,      // loadOp
      VK_ATTACHMENT_STORE_OP_STORE,     // storeOp
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,  // stencilLoadOp
      VK_ATTACHMENT_STORE_OP_DONT_CARE, // stencilStoreOp
      VK_IMAGE_LAYOUT_UNDEFINED,        // initialLayout
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR   // finalLayout
    }
  };

  VkAttachmentReference color_attachment_references[] = {
    {
      0,                                              // attachment
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL        // VkImageLayout
    }
  };

  VkSubpassDescription subpass_descriptions[] = {
    {
      0,                               // VkSubpassDescriptionFlags
      VK_PIPELINE_BIND_POINT_GRAPHICS, // VkPipelineBindPoint
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

  std::vector<VkSubpassDependency> dependencies = {
  {
    VK_SUBPASS_EXTERNAL,                           // srcSubpass
    0,                                             // dstSubpass
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // srcStageMask
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // dstStageMask
    VK_ACCESS_MEMORY_READ_BIT,                     // srcAccessMask
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,          // dstAccessMask
    VK_DEPENDENCY_BY_REGION_BIT                    // VkDependencyFlags  
  },
  {
    0,                                              // srcSubpass
    VK_SUBPASS_EXTERNAL,                            // dstSubpass
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // srcStageMask
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,           // dstStageMask
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,           // srcAccessMask
    VK_ACCESS_MEMORY_READ_BIT,                      // dstAccessMask
    VK_DEPENDENCY_BY_REGION_BIT                     // VkDependencyFlags
  }
  };

  VkRenderPassCreateInfo render_pass_create_info = {
    VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,        // VkStructureType
    nullptr,                                          // pNext
    0,                                                // VkRenderPassCreateFlags
    1,                                                // attachmentCount
    attachment_descriptions,                          // pAttachments
    1,                                                // subpassCount
    subpass_descriptions,                             // pSubpasses
    static_cast<uint32_t>(dependencies.size()),       // dependencyCount
    dependencies.data()                               // pDependencies
  };

  if (vkCreateRenderPass(m_device, &render_pass_create_info, nullptr, &m_render_pass) != VK_SUCCESS)
  {
    assert("Could not create render pass!", "Vulkan", Assert::Error);
    return false;
  }
  return true;
}

bool VKVertexAttributes::CreatePipeline()
{
  VkShaderModule vertex_shader_module = CreateShaderModule("shaders/t4/t4_vert.spv");
  VkShaderModule fragment_shader_module = CreateShaderModule("shaders/t4/t4_frag.spv");

  if (!vertex_shader_module || !fragment_shader_module)
    return false;

  VkPipelineShaderStageCreateInfo shader_stage_create_infos[2] = {
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

  VkVertexInputBindingDescription vertex_binding_descriptions[1] = {
    {
      0,                          // binding
      sizeof(VertexData),         // stride
      VK_VERTEX_INPUT_RATE_VERTEX // VkVertexInputRate
    }
  };

  VkVertexInputAttributeDescription vertex_attribute_descriptions[2] = {
    {
      0,                                      // location
      vertex_binding_descriptions[0].binding, // binding
      VK_FORMAT_R32G32B32A32_SFLOAT,          // format
      offsetof(struct VertexData, x)          // offset
    },
    {
      1,
      vertex_binding_descriptions[0].binding,
      VK_FORMAT_R32G32B32A32_SFLOAT,
      offsetof(struct VertexData, r)
    }
  };

  VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // VkStructureType
      nullptr,                                                   // pNext
      0,                                                         // VkPipelineVertexInputStateCreateFlags
      1,                                                         // vertexBindingDescriptionCount
      vertex_binding_descriptions,                               // const VkVertexInputBindingDescription*
      2,                                                         // vertexAttributeDescriptionCount
      vertex_attribute_descriptions                              // const VkVertexInputAttributeDescription*
  };

  VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {
    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, // VkStructureType
    nullptr,                                                     // pNext
    0,                                                           // VkPipelineInputAssemblyStateCreateFlags
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,                         // VkPrimitiveTopology
    VK_FALSE                                                     // primitiveRestartEnable
  };

  VkPipelineViewportStateCreateInfo viewport_state_create_info = {
    VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, // VkStructureType
    nullptr,                                               // pNext
    0,                                                     // VkPipelineViewportStateCreateFlags
    1,                                                     // viewportCount
    nullptr,                                               // pViewports
    1,                                                     // scissorCount
    nullptr                                                // pScissors
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

  constexpr uint32_t dynamic_state_count = 2;
  constexpr VkDynamicState dynamic_states[dynamic_state_count] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };

  VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
    VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, // VkStructureType
    nullptr,                                              // pNext
    0,                                                    // VkPipelineDynamicStateCreateFlags
    dynamic_state_count,                                  // dynamicStateCount
    dynamic_states                                        // pDynamicStates
  };

  VkPipelineLayout pipeline_layout = CreatePipelineLayout();
  if (!pipeline_layout)
    return false;

  VkGraphicsPipelineCreateInfo pipeline_create_info = {
    VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,         // VkStructureType
    nullptr,                                                 // pNext
    0,                                                       // VkPipelineCreateFlags
    2,                                                       // stageCount
    shader_stage_create_infos,                               // pStages
    &vertex_input_state_create_info,                         // pVertexInputState
    &input_assembly_state_create_info,                       // pInputAssemblyState
    nullptr,                                                 // pTessellationState
    &viewport_state_create_info,                             // pViewportState
    &rasterization_state_create_info,                        // pRasterizationState
    &multisample_state_create_info,                          // pMultisampleState
    nullptr,                                                 // pDepthStencilState
    &color_blend_state_create_info,                          // pColorBlendState
    &dynamic_state_create_info,                              // pDynamicState
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

VkShaderModule VKVertexAttributes::CreateShaderModule(const char* file_name)
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

VkPipelineLayout VKVertexAttributes::CreatePipelineLayout()
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

bool VKVertexAttributes::Update()
{
  VirtualFrame& current_virtual_frame = m_virtual_frames[m_current_virtual_frame_index];
  uint32_t      acquired_image_index;
  m_current_virtual_frame_index = (m_current_virtual_frame_index + 1) % m_virtual_frames_count;

  if (vkWaitForFences(m_device, 1, &current_virtual_frame.fence, VK_FALSE, 1000000000) != VK_SUCCESS)
  {
    assert("Problem occurred while waiting for fence!", "Vulkan", Assert::Error);
    return false;
  }
  vkResetFences(m_device, 1, &current_virtual_frame.fence);

  switch (vkAcquireNextImageKHR(m_device, m_swap_chain, UINT64_MAX, current_virtual_frame.image_available_semaphore, VK_NULL_HANDLE, &acquired_image_index))
  {
  case VK_SUCCESS:
  case VK_SUBOPTIMAL_KHR:
    break;
  case VK_ERROR_OUT_OF_DATE_KHR:
    return OnWindowSizeChanged();
  default:
    assert("Problem occurred during swap chain image acquisition!", "Vulkan", Assert::Error);
    return false;
  }

  if (!PrepareFrame(current_virtual_frame.command_buffer, acquired_image_index, current_virtual_frame.frame_buffer))
    return false;

  VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  
  VkSubmitInfo submit_info = {
    VK_STRUCTURE_TYPE_SUBMIT_INFO,                      // VkStructureType 
    nullptr,                                            // pNext
    1,                                                  // waitSemaphoreCount
    &current_virtual_frame.image_available_semaphore,   // pWaitSemaphores
    &wait_dst_stage_mask,                               // pWaitDstStageMask
    1,                                                  // commandBufferCount
    &current_virtual_frame.command_buffer,              // pCommandBuffers
    1,                                                  // signalSemaphoreCount
    &current_virtual_frame.finished_rendering_semaphore // pSignalSemaphores
  };

  if (vkQueueSubmit(m_graphics_queue, 1, &submit_info, current_virtual_frame.fence) != VK_SUCCESS)
  {
    assert("Failed to submit to graphics queue!", "Vulkan", Assert::Error);
    return false;
  }

  VkPresentInfoKHR present_info = {
    VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,                  // VkStructureType
    nullptr,                                             // pNext
    1,                                                   // waitSemaphoreCount
    &current_virtual_frame.finished_rendering_semaphore, // pWaitSemaphores
    1,                                                   // swapchainCount
    &m_swap_chain,                                       // pSwapchains
    &acquired_image_index,                               // pImageIndices
    nullptr                                              // pResults
  };
  
  switch (vkQueuePresentKHR(m_present_queue, &present_info))
  {
  case VK_SUCCESS:
    break;
  case VK_ERROR_OUT_OF_DATE_KHR:
  case VK_SUBOPTIMAL_KHR:
    return OnWindowSizeChanged();
  default:
    assert("Problem occurred during image presentation!", "Vulkan", Assert::Error);
    return false;
  }

  return true;
}

bool VKVertexAttributes::PrepareFrame(VkCommandBuffer command_buffer, const size_t& image_index, VkFramebuffer& frame_buffer)
{
  if (!CreateFrameBuffer(frame_buffer, m_swap_chain_info.image_views[image_index]))
    return false;

  VkCommandBufferBeginInfo command_buffer_begin_info = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, // VkStructureType
    nullptr,                                     // pNext
    VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // VkCommandBufferUsageFlags
    nullptr                                      // pInheritanceInfo
  };

  vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

  VkImageSubresourceRange image_subresource_range = {
    VK_IMAGE_ASPECT_COLOR_BIT,  // aspectMask
    0,                          // baseMipLevel
    1,                          // levelCount
    0,                          // baseArrayLayer
    1                           // layerCount
  };

  if (m_graphics_queue != m_present_queue)
  {
    VkImageMemoryBarrier barrier_from_present_to_draw = {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // VkStructureType
      nullptr,                                // pNext
      VK_ACCESS_MEMORY_READ_BIT,              // srcAccessMask
      VK_ACCESS_MEMORY_READ_BIT,              // dstAccessMask
      VK_IMAGE_LAYOUT_UNDEFINED,              // oldLayout
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,        // newLayout
      m_present_queue_family_index,           // srcQueueFamilyIndex
      m_graphics_queue_family_index,          // dstQueueFamilyIndex
      m_swap_chain_info.images[image_index],  // image
      image_subresource_range                 // subresourceRange
    };
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier_from_present_to_draw);
  }

  VkClearValue clear_value = {{ 1.0f, 0.8f, 0.4f, 0.0f }};

  VkRenderPassBeginInfo render_pass_begin_info = {
    VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // VkStructureType
    nullptr,                                   // pNext
    m_render_pass,                             // VkRenderPass 
    frame_buffer,                              // VkFramebuffer
    {                                          // renderArea
      {                                        // VkOffset2D
        0,                                       // x
        0                                        // y
      },                                       
      m_swap_chain_info.extent             
    },                                        
    1,                                         // clearValueCount
    &clear_value                               // pClearValues
  };
  
  vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

  VkViewport viewport = {
    0.0f,                                                // x
    0.0f,                                                // y
    static_cast<float>(m_swap_chain_info.extent.width),  // width
    static_cast<float>(m_swap_chain_info.extent.height), // height
    0.0f,                                                // minDepth, depth range is [0, 1)
    1.0f                                                 // maxDepth
  };

  VkRect2D scissor = {
    {    // VkOffset2D
      0,   // x
      0    // y
    },
    m_swap_chain_info.extent
  };

  vkCmdSetViewport(command_buffer, 0, 1, &viewport);
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(command_buffer, 0, 1, &m_vertex_buffer, &offset);
  
  vkCmdDraw(command_buffer, m_vertex_buffer_info.count, 1, 0, 0);

  vkCmdEndRenderPass(command_buffer);

  if (m_graphics_queue != m_present_queue)
  {
    VkImageMemoryBarrier barrier_from_draw_to_present = {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // VkStructureType
      nullptr,                                // pNext
      VK_ACCESS_MEMORY_READ_BIT,              // srcAccessMask
      VK_ACCESS_MEMORY_READ_BIT,              // dstAccessMask
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,        // oldLayout
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,        // newLayout
      m_graphics_queue_family_index,          // srcQueueFamilyIndex
      m_present_queue_family_index,           // dstQueueFamilyIndex
      m_swap_chain_info.images[image_index],  // image
      image_subresource_range                 // subresourceRange
    };
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier_from_draw_to_present);
  }

  if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
  {
    assert("Could not record command buffer!", "Vulkan", Assert::Error);
    return false;
  }

  return true;
}

bool VKVertexAttributes::CreateFrameBuffer(VkFramebuffer& frame_buffer, const VkImageView image_view)
{
  if (frame_buffer != VK_NULL_HANDLE)
  {
    vkDestroyFramebuffer(m_device, frame_buffer, nullptr);
    frame_buffer = VK_NULL_HANDLE;
  }
  
  VkFramebufferCreateInfo framebuffer_create_info = {
    VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, // VkStructureType
    nullptr,                                   // pNext
    0,                                         // VkFramebufferCreateFlags
    m_render_pass,                             // VkRenderPass
    1,                                         // attachmentCount
    &image_view,                               // pAttachments
    m_swap_chain_info.extent.width,            // width
    m_swap_chain_info.extent.height,           // height
    1                                          // layers
  };

  if (vkCreateFramebuffer(m_device, &framebuffer_create_info, nullptr, &frame_buffer) != VK_SUCCESS)
  {
    assert("Could not create a framebuffer!", "Vulkan", Assert::Error);
    return false;
  }

  return true;
}

void VKVertexAttributes::Terminate()
{
  Clear();

  //warning! all releasing should be in order!
  if (m_device != VK_NULL_HANDLE)
  {
    vkDeviceWaitIdle(m_device);

    if (m_swap_chain != VK_NULL_HANDLE)
    {
      vkDestroySwapchainKHR(m_device, m_swap_chain, nullptr);
      m_swap_chain = VK_NULL_HANDLE;
    }

    if (m_vertex_buffer_info.memory != VK_NULL_HANDLE)
    {
      vkFreeMemory(m_device, m_vertex_buffer_info.memory, nullptr);
      m_vertex_buffer_info.memory = VK_NULL_HANDLE;
    }

    if (m_vertex_buffer != VK_NULL_HANDLE)
    {
      vkDestroyBuffer(m_device, m_vertex_buffer, nullptr);
      m_vertex_buffer = VK_NULL_HANDLE;
    }

    if (m_staging_buffer_info.memory != VK_NULL_HANDLE)
    {
      vkFreeMemory(m_device, m_staging_buffer_info.memory, nullptr);
      m_staging_buffer_info.memory = VK_NULL_HANDLE;
    }

    if (m_staging_buffer != VK_NULL_HANDLE)
    {
      vkDestroyBuffer(m_device, m_staging_buffer, nullptr);
      m_staging_buffer = VK_NULL_HANDLE;
    }

    if (m_graphics_command_pool != VK_NULL_HANDLE)
    {
      vkDestroyCommandPool(m_device, m_graphics_command_pool, nullptr);
      m_graphics_command_pool = VK_NULL_HANDLE;
    }

    for (VirtualFrame& virtual_frame : m_virtual_frames)
    {
      vkDestroySemaphore(m_device, virtual_frame.image_available_semaphore, nullptr);
      vkDestroySemaphore(m_device, virtual_frame.finished_rendering_semaphore, nullptr);
      virtual_frame.image_available_semaphore = VK_NULL_HANDLE;
      virtual_frame.finished_rendering_semaphore = VK_NULL_HANDLE;
      vkDestroyFence(m_device, virtual_frame.fence, nullptr);
      virtual_frame.fence = VK_NULL_HANDLE;
      vkDestroyFramebuffer(m_device, virtual_frame.frame_buffer, nullptr);
      virtual_frame.frame_buffer = VK_NULL_HANDLE;
    }
  }
}

#endif