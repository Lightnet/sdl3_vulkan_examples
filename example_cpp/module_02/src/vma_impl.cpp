// vma_impl.cpp
// VMA configuration for Volk
#define VMA_STATIC_VULKAN_FUNCTIONS 0    // Disable static Vulkan functions
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1   // Enable dynamic loading with Volk
#define VMA_VULKAN_VERSION 1004000       // Vulkan 1.4

// Define VMA implementation here
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>