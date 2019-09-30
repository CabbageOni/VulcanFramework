#pragma once

#include "using_vk_mode.setting"

class VKBase
{
private:
  virtual bool BaseInitialize() final;
  virtual bool LoadVulkanLibrary() final;
  virtual bool LoadExportedEntryPoints() final;
  virtual bool LoadGlobalLevelEntryPoints() final;
  virtual bool CreateInstance();
  virtual bool LoadInstanceLevelEntryPoints() final;
  virtual bool CreatePresentationSurface() final;
  virtual bool CreateDevice();
  bool CheckPhysicalDeviceProperties(VkPhysicalDevice physical_device, uint32_t &queue_family_index);
  virtual bool LoadDeviceLevelEntryPoints() final;
  virtual bool Initialize();
  virtual bool Update();
  virtual void Terminate();
  virtual void BaseTerminate() final;

protected:
  HMODULE m_vulkan_library;
  VkInstance m_instance;
  VkPhysicalDevice m_physical_device;
  VkDevice m_device;
  VkSurfaceKHR m_presentation_surface;

  virtual bool CheckExtensionAvailability(const char* extension_name, const std::vector<VkExtensionProperties>& available_extensions) final;

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
  friend bool Update();
  friend void Terminate();
};