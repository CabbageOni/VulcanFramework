//notice this doesn't have #pragma_once!                  

#include "vkbase.h"

#if VK_CURRENT_MODE == VK_SWAPCHAIN
#include "vkswapchain.h"
#elif VK_CURRENT_MODE == VK_FIRST_TRIANGLE
#include "vkfirsttriangle.h"
#elif VK_CURRENT_MODE == VK_VERTEX_ATTRIBUTES
#include "vkvertexattributes.h"
#endif
