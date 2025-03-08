#ifndef VMAUSAGE_H
#define VMAUSAGE_H
// #define VK_NO_PROTOTYPES
//#include <volk.h> // Include Volk first

// VMA configuration for Volk
#define VMA_STATIC_VULKAN_FUNCTIONS 0    // Disable static Vulkan functions
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1   // Enable dynamic loading with Volk
#define VMA_VULKAN_VERSION 1004000       // Vulkan 1.4

// Define VMA implementation here
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

// Function declaration
//bool initialize_vma(VmaAllocator& allocator, VkPhysicalDevice physicalDevice, VkDevice device, VkInstance instance);

#endif