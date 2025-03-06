#ifndef VSDL_TYPES_H
#define VSDL_TYPES_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>  // Explicit Vulkan support
#include <vulkan/vulkan.h>
#include <stdio.h>

#define LOG(msg, ...) printf("[LOG] " msg "\n", ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) printf("[ERROR] " msg "\n", ##__VA_ARGS__)

typedef struct {
    SDL_Window* window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkImage* swapchainImages;
    VkImageView* swapchainImageViews;
    uint32_t swapchainImageCount;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkFramebuffer* framebuffers;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
} VsdlContext;

extern VsdlContext g_context;

#endif