#ifndef VSDL_TYPES_H
#define VSDL_TYPES_H

#include <SDL3/SDL.h>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <volk.h>
#include <vk_mem_alloc.h>

typedef struct  {
    float pos[2];  // x, y
    float color[3]; // r, g, b
} Vertex;

typedef struct  {
    SDL_Window* window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    uint32_t graphicsFamily;
    VmaAllocator allocator;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkImage* swapchainImages;
    uint32_t swapchainImageCount;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    VkImageView* swapchainImageViews;
    uint32_t swapchainImageViewCount;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkFramebuffer* framebuffers;
    uint32_t framebufferCount;
    VkCommandPool commandPool;
    VkBuffer vertexBuffer;
    VmaAllocation vertexBufferAllocation;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence frameFence;
} VSDL_Context;

#endif