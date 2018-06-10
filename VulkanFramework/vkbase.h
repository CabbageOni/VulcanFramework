#pragma once

class VKBase
{
private:
  bool CheckPhysicalDeviceProperties(VkPhysicalDevice physical_device, uint32_t &queue_family_index);

protected:
  HMODULE m_vulkan_library;
  VkInstance m_instance;
  VkDevice m_device;
  VkPhysicalDevice m_physical_device;

  virtual bool LoadVulkanLibrary() final;
  virtual bool LoadExportedEntryPoints() final;
  virtual bool LoadGlobalLevelEntryPoints() final;
  virtual bool CreateInstance();
  virtual bool LoadInstanceLevelEntryPoints() final;
  virtual bool CreateDevice();
  virtual bool LoadDeviceLevelEntryPoints() final;
  virtual bool GetDeviceQueue();

  virtual bool Initialize();
  virtual void Update();
  virtual void Terminate();

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
};