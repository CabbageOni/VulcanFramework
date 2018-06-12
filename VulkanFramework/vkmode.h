//notice this doesn't have #pragma_once!                  

#include "vkbase.h"

#include "using_vk_mode.setting"

#if VK_CURRENT_MODE == VK_SWAPCHAIN
#include "vkswapchain.h"
#endif
