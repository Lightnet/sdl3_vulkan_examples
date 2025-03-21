// this will not work.
// Some features of C++14 used. STL containers, RTTI, or C++ exceptions are not used
// https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/tree/master
// 
#define VMA_STATIC_VULKAN_FUNCTIONS 0    // Disable static Vulkan functions
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1   // Enable dynamic loading with Volk
#define VMA_VULKAN_VERSION 1004000       // Vulkan 1.4

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>