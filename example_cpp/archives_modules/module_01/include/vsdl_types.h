#ifndef VSDL_TYPES_H
#define VSDL_TYPES_H

//#define VMA_VULKAN_VERSION 1004000 // Vulkan 1.4?

//#define VMA_IMPLEMENTATION
//#define VMA_STATIC_VULKAN_FUNCTIONS VK_TRUE
//#define VMA_DEBUG_GLOBAL_MUTEX VK_TRUE
//#define VMA_RECORDING_ENABLED PICOVK_ENABLE_VALIDATION
//#define VMA_VULKAN_VERSION 1002000 // Vulkan 1.2
//#define VMA_VULKAN_VERSION 1004000 // Vulkan 1.4?
//#define VMA_STATIC_VULKAN_FUNCTIONS 0
//#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
//#include "vmausage.h"
//#include <vk_mem_alloc.h> // Include VMA header
#include <vector>

struct VSDL_Context {
    SDL_Window* window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImageView> swapchainImageViews;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkFramebuffer> framebuffers;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
};

#endif