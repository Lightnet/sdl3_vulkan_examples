#ifndef VULKAN_INIT_H
#define VULKAN_INIT_H

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "vsdl_types.h" // Add this for VulkanContext

#ifdef __cplusplus
extern "C" {
#endif

void init_vulkan(SDL_Window* window, VkInstance* instance, VkPhysicalDevice* physicalDevice, VkDevice* device, 
                 VkQueue* graphicsQueue, VkSurfaceKHR* surface, VkSwapchainKHR* swapchain, uint32_t* imageCount,
                 VkImage** swapchainImages, VkImageView** swapchainImageViews, uint32_t* graphicsQueueFamilyIndex);

extern VmaAllocator allocator;

#ifdef __cplusplus
}
#endif

#endif