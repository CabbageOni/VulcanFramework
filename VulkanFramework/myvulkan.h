#pragma once

#include <Windows.h>
#include <vector>

//defines
#define VK_USE_PLATFORM_WIN32_KHR
#define USE_SWAPCHAIN_EXTENSIONS
#include "resources\vulkan\vulkan.h"

extern class Vulkan
{
private:
  HMODULE m_vulkan_library;
  VkInstance m_instance;
  //belows should be one per device, but for now they are here since I'm using only one device
  VkDevice m_device;
  VkPhysicalDevice m_physical_device;
  VkSurfaceKHR  m_presentation_surface;
  uint32_t m_graphics_queue_family_index;
  uint32_t m_present_queue_family_index;
  VkQueue m_graphics_queue;
  VkQueue m_present_queue;
  VkSemaphore m_image_available_semaphore;
  VkSemaphore m_rendering_finished_semaphore;

  bool LoadVulkanLibrary();
  bool LoadExportedEntryPoints();
  bool LoadGlobalLevelEntryPoints();
  bool CreateInstance();
  bool CheckExtensionAvailability(const char* extension_name, const std::vector<VkExtensionProperties>& available_extensions);
  bool LoadInstanceLevelEntryPoints();
  bool CreateDevice();
  bool CheckPhysicalDeviceProperties(VkPhysicalDevice physical_device,
                                    uint32_t& selected_graphics_queue_family_index,
                                    uint32_t &selected_present_queue_family_index);
  bool LoadDeviceLevelEntryPoints();
  bool GetDeviceQueue();

  //extensions required
  bool CreatePresentationSurface();
  bool CreateSemaphores();
  bool CreateSwapChain();

  bool Initialize();
  void Update();
  void Terminate();

  //Vulkan functions
  #define LOAD_EXPORTED(func) PFN_##func func;
  #define LOAD_GLOBAL_LEVEL(func) PFN_##func func;
  #define LOAD_INSTANCE_LEVEL(func) PFN_##func func;
  #define LOAD_DEVICE_LEVEL(func) PFN_##func func;
  #include "vulkanFunctions.inl"
  #undef LOAD_EXPORTED
  #undef LOAD_GLOBAL_LEVEL
  #undef LOAD_INSTANCE_LEVEL
  #undef LOAD_DEVICE_LEVEL

public:
  friend bool Initialize();
  friend void Update();
  friend void Terminate();

} vulkan;