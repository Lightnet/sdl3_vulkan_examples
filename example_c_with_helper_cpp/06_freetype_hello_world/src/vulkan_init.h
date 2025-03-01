#ifndef VULKAN_INIT_H
#define VULKAN_INIT_H

// Header for Vulkan initialization functions and globals
// Purpose: Declares Vulkan setup function and VMA allocator for use in main.c and vulkan_init.cpp

#include <SDL3/SDL.h>         // Defines SDL_Window for window creation
#include <vulkan/vulkan.h>    // Core Vulkan types (VkInstance, VkDevice, etc.)
#include <vk_mem_alloc.h>     // VMA allocator type (VmaAllocator)

// Ensure C linkage for compatibility between C (main.c) and C++ (vulkan_init.cpp)
#ifdef __cplusplus
extern "C" {
#endif

// Initializes Vulkan core components
// Parameters:
// - window: SDL window for surface creation
// - instance: Pointer to Vulkan instance handle (output)
// - physicalDevice: Pointer to physical device handle (output)
// - device: Pointer to logical device handle (output)
// - graphicsQueue: Pointer to graphics queue handle (output)
// - surface: Pointer to surface handle (output)
// - swapchain: Pointer to swapchain handle (output)
// - imageCount: Pointer to number of swapchain images (output)
// - swapchainImages: Pointer to array of swapchain images (output)
// - swapchainImageViews: Pointer to array of swapchain image views (output)
// - graphicsQueueFamilyIndex: Pointer to graphics queue family index (output)
void init_vulkan(SDL_Window* window, VkInstance* instance, VkPhysicalDevice* physicalDevice, VkDevice* device, 
                 VkQueue* graphicsQueue, VkSurfaceKHR* surface, VkSwapchainKHR* swapchain, uint32_t* imageCount,
                 VkImage** swapchainImages, VkImageView** swapchainImageViews, uint32_t* graphicsQueueFamilyIndex);

// Global VMA allocator for memory management
// Defined in vulkan_init.cpp, used across the app for buffers and images
extern VmaAllocator allocator;

#ifdef __cplusplus
}
#endif

#endif // VULKAN_INIT_H

// Notes for Users:
// - Include this header in both main.c and vulkan_init.cpp to ensure consistent declarations.
// - All parameters are pointers to allow modification of vkCtx fields in main.c.
// - allocator is global; ensure it’s destroyed with vmaDestroyAllocator before vkDestroyDevice.