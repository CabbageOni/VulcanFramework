#pragma once

#include <Windows.h>

#include "resources\vulkan\vulkan.h"

extern class Vulkan
{
private:
  HMODULE m_vulkanLibrary;
  VkInstance m_instance;

  bool LoadVulkanLibrary();
  bool LoadExportedEntryPoints();
  bool LoadGlobalLevelEntryPoints();
  bool CreateInstance();
  bool LoadInstanceLevelEntryPoints();
  bool CreateDevice();

  bool Initialize();

  //Vulkan functions
  #define LOAD_EXPORTED(func) PFN_##func func;
  #define LOAD_GLOBAL_LEVEL(func) PFN_##func func;
  #define LOAD_INSTANCE_LEVEL(func) PFN_##func func;
  #include "vulkanFunctions.inl"
  #undef LOAD_EXPORTED
  #undef LOAD_GLOBAL_LEVEL
  #undef LOAD_INSTANCE_LEVEL
public:

  friend bool Initialize();

} vulkan;