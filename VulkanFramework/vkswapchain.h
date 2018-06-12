#pragma once

#include <vector>

#include "vkbase.h"

class VKSwapChain : public VKBase
{
private:
  VkPhysicalDevice m_physical_device;
  VkSurfaceKHR  m_presentation_surface;
  uint32_t m_graphics_queue_family_index;
  uint32_t m_present_queue_family_index;
  VkQueue m_graphics_queue;
  VkQueue m_present_queue;
  VkSemaphore m_image_available_semaphore;
  VkSemaphore m_rendering_finished_semaphore;
  VkSwapchainKHR m_swap_chain;
  std::vector<VkCommandBuffer> m_present_queue_cmd_buffers;
  VkCommandPool m_present_queue_cmd_pool;

  virtual bool CreateInstance() override;
  bool CheckExtensionAvailability(const char* extension_name, const std::vector<VkExtensionProperties>& available_extensions);
  virtual bool CreateDevice() override;
  bool CheckPhysicalDeviceProperties(VkPhysicalDevice physical_device,
                                    uint32_t& selected_graphics_queue_family_index,
                                    uint32_t &selected_present_queue_family_index);
  bool GetDeviceQueue() override;
  bool OnWindowSizeChanged();
  void Clear();

  //extensions required
  bool CreatePresentationSurface();
  bool CreateSemaphores();
  bool CreateSwapChain();
  bool CreateCommandBuffers();
  bool RecordCommandBuffers();

  uint32_t GetSwapChainNumImages(VkSurfaceCapabilitiesKHR &surface_capabilities);
  VkSurfaceFormatKHR GetSwapChainFormat(std::vector<VkSurfaceFormatKHR> &surface_formats);
  VkExtent2D GetSwapChainExtent(VkSurfaceCapabilitiesKHR &surface_capabilities);
  VkImageUsageFlags GetSwapChainUsageFlags(VkSurfaceCapabilitiesKHR& surface_capabilities);
  VkSurfaceTransformFlagBitsKHR GetSwapChainTransform(VkSurfaceCapabilitiesKHR& surface_capabilities);
  VkPresentModeKHR GetSwapChainPresentMode(std::vector<VkPresentModeKHR>& present_modes);

  virtual bool Initialize() override;
  virtual void Update() override;
  virtual void Terminate() override;

public:

  friend bool Initialize();
  friend void Update();
  friend void Terminate();
};