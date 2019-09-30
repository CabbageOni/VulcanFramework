#pragma once

#include <vector>

#include "vkbase.h"

class VKSwapChain : public VKBase
{
private:
  uint32_t                     m_graphics_queue_family_index;
  uint32_t                     m_present_queue_family_index;
  VkQueue                      m_graphics_queue;
  VkQueue                      m_present_queue;
  VkSemaphore                  m_image_available_semaphore;
  VkSemaphore                  m_rendering_finished_semaphore;
  VkSwapchainKHR               m_swap_chain;
  VkSurfaceFormatKHR           m_swap_chain_format;
  std::vector<VkCommandBuffer> m_present_queue_cmd_buffers;
  VkCommandPool                m_present_queue_cmd_pool;

  virtual bool CreateDevice() override;
  bool CheckPhysicalDeviceProperties(VkPhysicalDevice physical_device,
                                    uint32_t& selected_graphics_queue_family_index,
                                    uint32_t &selected_present_queue_family_index);
  bool GetDeviceQueue();
  bool OnWindowSizeChanged();
  void Clear();

  //extensions required
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
  virtual bool Update() override;
  virtual void Terminate() override;

public:

  friend bool Initialize();
  friend bool Update();
  friend void Terminate();
};