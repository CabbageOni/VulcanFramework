#pragma once

#include "vkbase.h"

class VKVertexAttributes : public VKBase
{
private:
  uint32_t m_graphics_queue_family_index;
  uint32_t m_present_queue_family_index;
  VkQueue  m_graphics_queue;
  VkQueue  m_present_queue;

  virtual bool Initialize() override;
  virtual bool CreateDevice() override;
  bool CheckPhysicalDeviceProperties(VkPhysicalDevice physical_device,
    uint32_t& selected_graphics_queue_family_index,
    uint32_t &selected_present_queue_family_index);
  bool GetDeviceQueue();
  virtual void Terminate() override;

public:
  friend bool Initialize();
  friend void Update();
  friend void Terminate();
};